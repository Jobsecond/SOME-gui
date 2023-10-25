#include <fstream>
#include <chrono>
#include <stdexcept>

#include <QColor>

#include <sndfile.hh>
#if defined(SOME_ENABLE_R8BRAIN)
# include <r8bbase.h>
# include <CDSPResampler.h>
#elif defined(SOME_ENABLE_SAMPLERATE)
# include <samplerate.h>
#endif
#include <MidiFile.h>

#include "Worker.h"
#include "Slicer/Slicer.h"
#include "Inference/SOMEInference.h"

#define DIVIDE_CEIL(x, y)  (((x) % (y)) ? (((x) / (y)) + 1) : ((x) / (y)))
#define MIN_VALUE(a,b)            (((a) < (b)) ? (a) : (b))

Worker::Worker(const QString &modelPath,
               const QString &audioPath,
               double tempo,
               const QString &outPath,
               some::ExecutionProvider ep,
               int deviceIndex,
               int batchSize,
               QObject *parent) : QThread(parent), m_modelPath(modelPath),
               m_audioPath(audioPath), m_tempo(tempo), m_outPath(outPath),
               m_deviceIndex(deviceIndex), m_ep(ep), m_batchSize(batchSize)
               {}


void Worker::run() {
    using namespace some;
    auto benchmarkStart = std::chrono::steady_clock::now();

    // Step: initialize Ort Session
    logMsgInfo("Initializing session...");
    SOMEInference someInference(m_modelPath);
    connect(&someInference, &Inference::logMsgInfo, [this](const QString &msg) {
        Q_EMIT logMsgInfo(msg);
    });
    connect(&someInference, &Inference::logMsgError, [this](const QString &msg) {
        Q_EMIT logMsgError(msg);
    });
    someInference.initSession(m_ep, m_deviceIndex);

    if (!someInference.hasSession()) {
        Q_EMIT logMsgError("Session initialization failed.");
        return;
    }
    logMsgInfo("Session initialization succeed.");


    // Step: load audio
    logMsgInfo("Loading audio...");

    SndfileHandle sf(m_audioPath
#ifdef _WIN32
            .toStdWString().c_str()
#else
            .toStdString()
#endif
            );
    if (sf.error() != SF_ERR_NO_ERROR)
    {
        Q_EMIT logMsgError(QString("Sndfile error: %1").arg(sf.strError()));
        return;
    }
    auto channels = sf.channels();
    auto sampleRate = sf.samplerate();
    constexpr int targetSampleRate = 44100;

    auto frames = sf.frames();
    if ((channels <= 0) || (sampleRate <= 0)) {
        Q_EMIT logMsgError("Audio load failed.");
        return;
    }

    if (frames == 0) {
        Q_EMIT logMsgError("Audio is empty!");
        return;
    }

    constexpr long long maxAllowedLength = 20 * 60;
    if (frames >= sampleRate * maxAllowedLength) {
        Q_EMIT logMsgError("Error: the input audio is too long (>= 20 minutes).");
        return;
    }
    auto samples = frames * channels;
    std::vector<float> waveform;
    try {
        waveform.resize(samples);
    }
    catch (const std::bad_alloc &e) {
        Q_EMIT logMsgError(QString("Failed to allocate memory for audio: ") + e.what());
        return;
    }

    auto itemsRead = sf.read(waveform.data(), samples);
    if (itemsRead <= 0) {
        Q_EMIT logMsgError("Can't read audio file!");
        return;
    }
    samples = MIN_VALUE(samples, itemsRead);
    frames = MIN_VALUE(frames, samples / channels);

    // Step: convert audio
    // Convert to mono in-place
    if (channels > 1) {
        for (size_t i = 0; i < frames; i++) {
            float s = 0;
            for (int j = 0; j < channels; j++) {
                s += waveform[i * channels + j] / static_cast<float>(channels);
            }
            waveform[i] = s;
        }
        waveform.resize(frames);
        samples = frames;
    }

    // Convert sample rate
    if (sampleRate != targetSampleRate) {
#if defined(SOME_ENABLE_R8BRAIN)
        logMsgInfo(QString("Converting sample rate from %2 Hz to %1 Hz")
                           .arg(targetSampleRate).arg(sampleRate));
        constexpr int inBufferSize = 1024;
        std::array<double, inBufferSize> inBuffer {};

        r8b::CDSPResampler resampler(sampleRate, targetSampleRate, inBufferSize);

        auto targetFrames = frames * targetSampleRate / sampleRate;
        auto numBatches = frames / inBufferSize;
        auto remain = frames % inBufferSize;  // within int range
        if (remain > 0) {
            ++numBatches;
        }
        double *outBuffer;
        std::vector<float> outVec;
        outVec.reserve(targetFrames);
        decltype(numBatches) currentBatch = 0;

        while (currentBatch < numBatches) {
            int ReadCount = ((currentBatch == numBatches) && (remain > 0)) ? static_cast<int>(remain) : inBufferSize;
            for (int i = 0; i < ReadCount; ++i) {
                inBuffer[i] = waveform[currentBatch * inBufferSize + i];
            }
            int writeCount = resampler.process(
                    inBuffer.data(),
                    inBufferSize,
                    outBuffer
            );
            for (int i = 0; i < writeCount; ++i) {
                outVec.push_back(static_cast<float>(outBuffer[i]));
            }
            ++currentBatch;
        }
        frames = targetFrames;
        samples = frames;
        std::swap(waveform, outVec);
#elif defined(SOME_ENABLE_SAMPLERATE);
        logMsgInfo(QString("Converting sample rate from %2 Hz to %1 Hz")
                            .arg(targetSampleRate).arg(sampleRate));
        double conversionRatio = 1.0 * targetSampleRate / sampleRate;

        SRC_DATA srcData;
        srcData.src_ratio = conversionRatio;
        srcData.data_in = waveform.data();
        srcData.data_out = nullptr;
        srcData.input_frames = frames;
        srcData.output_frames = 0;

        src_simple(&srcData, SRC_SINC_FASTEST, 1);
        auto targetFrames = frames * targetSampleRate / sampleRate;

        std::vector<float> outBuffer(targetFrames);
        srcData.data_out = outBuffer.data();
        srcData.output_frames = targetFrames;

        src_simple(&srcData, SRC_SINC_FASTEST, 1);
        frames = srcData.output_frames_gen;
        outBuffer.resize(frames);
        samples = frames;
        std::swap(waveform, outBuffer);
#else
        logMsgError(QString("Please convert the sample rate to %1 Hz first! Actual sample rate: %2 Hz")
                .arg(targetSampleRate).arg(sampleRate));
        return;
#endif
    }

    logMsgInfo("Slicing audio...");
    Slicer slicer(targetSampleRate, -40.0, 5000, 300, 20, 1000);
    auto markers = slicer.slice(waveform, 1);

    smf::MidiFile midi;
    constexpr int NOTE_VELOCITY = 64;
    auto trackId = midi.addTrack();
    midi.addTempo(trackId, 0, m_tempo);
    auto mul = m_tempo * midi.getTicksPerQuarterNote() / 60;

    if (markers.empty()) {
        logMsgError("Run slicer failed.");
        return;
    }
    logMsgInfo(QString("Slicing succeed. Total chunks: %1").arg(markers.size()));

    int currentMarkerIndex = 0;

    for (const auto [beginFrame, endFrame] : markers) {
        auto currentAudioDuration = (endFrame - beginFrame) * 1000 / targetSampleRate;
        logMsgInfo(QString("Inferring audio chunk %1/%2, length: %3 s")
                .arg(currentMarkerIndex + 1)
                .arg(markers.size())
                .arg(QString::number(currentAudioDuration / 1000.0, 'f', 3)));
        auto notes = someInference.infer(waveform, beginFrame, endFrame - beginFrame);
        auto notesSize = notes.note_midi.size();
        if (notesSize != notes.note_dur.size() || notesSize != notes.note_rest.size()) {
            logMsgError("The sizes of `note_midi`, `note_dur`, `note_rest` do not match!");
            return;
        }
        logMsgInfo(QString("Audio chunk %1/%2 inference complete.").arg(currentMarkerIndex + 1).arg(markers.size()));
        float cumSum = 0.0f, cumSumPrev = 0.0f;
        int offset = std::lround(beginFrame * mul / sampleRate);

        int start = offset;
        for (size_t i = 0; i < notesSize; ++i) {
            int noteMidi = std::lround(notes.note_midi[i]);
            cumSumPrev = cumSum;
            cumSum += notes.note_dur[i];
            int noteTick = std::lround(cumSum * mul) - std::lround(cumSumPrev * mul);
            bool noteRest = notes.note_rest[i];

            int end = start + noteTick;

            if (currentMarkerIndex < markers.size() - 1) {
                auto nextBeginFrame = markers[currentMarkerIndex + 1].first;
                int nextOffset = std::lround(nextBeginFrame * mul / sampleRate);
                if (end > nextOffset) {
                    end = nextOffset;
                }
            }
            if (start < end && !noteRest) {
                midi.addNoteOn(trackId, start, 0, noteMidi, NOTE_VELOCITY);
                midi.addNoteOff(trackId, end, 0, noteMidi);
            }
            start = end;
        }
        ++currentMarkerIndex;
    }

    std::ofstream outMidiFile(m_outPath
#ifdef _WIN32
            .toStdWString()
#else
            .toStdString()
#endif
    , std::ios::binary);

    midi.write(outMidiFile);

    auto benchmarkTimeEnd = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(benchmarkTimeEnd - benchmarkStart).count();
    logMsgWithColor(QString("Task completed in %1 seconds.").arg(QString::number(duration / 1000.0, 'f', 3)), Qt::darkGreen);
}

Worker::Worker(QObject *parent) : QThread(parent), m_modelPath(),
                                  m_audioPath(), m_tempo(0), m_outPath() {}
