#include <cmath>

#include "Slicer.h"
#include "Slicer-inl.h"


Slicer::Slicer(int sr, double threshold, std::size_t minLength, std::size_t minInterval, std::size_t hopSize, std::size_t maxSilKept) {
    m_errCode = SlicerErrorCode::SLICER_OK;
    m_errMsg.clear();

    if ((!((minLength >= minInterval) && (minInterval >= hopSize))) || (maxSilKept < hopSize)) {
        // The following condition must be satisfied: m_minLength >= m_minInterval >= m_hopSize
        // The following condition must be satisfied: m_maxSilKept >= m_hopSize
        m_errCode = SlicerErrorCode::SLICER_INVALID_ARGUMENT;
        m_errMsg = "ValueError: The following conditions must be satisfied: "
                   "(min_length >= min_interval >= hop_size) and (max_sil_kept >= hop_size).";
        return;
    }

    if (sr <= 0) {
        m_errCode = SlicerErrorCode::SLICER_AUDIO_ERROR;
        m_errMsg = "AudioError: Invalid audio sample rate!";
        return;
    }

    constexpr std::size_t unitFactor = 1000;
    m_threshold = std::pow(10, threshold / 20.0);
    m_hopSize = divIntRound(hopSize * sr, unitFactor);
    m_winSize = std::min(divIntRound(minInterval * sr, unitFactor), m_hopSize * 4);
    m_minLength = divIntRound(minLength * sr, unitFactor * m_hopSize);
    m_minInterval = divIntRound(minInterval * sr, unitFactor * m_hopSize);
    m_maxSilKept = divIntRound(maxSilKept * sr, unitFactor * m_hopSize);
}


MarkerList Slicer::slice(const std::vector<float> &waveform, int channels)
{
    if (m_errCode == SlicerErrorCode::SLICER_INVALID_ARGUMENT)
    {
        return {};
    }

    auto frames = waveform.size() / channels;
    if (channels <= 0) {
        m_errCode = SLICER_AUDIO_ERROR;
        m_errMsg = "Invalid audio channel size!";
        return {};
    }
    if (frames <= 0) {
        m_errCode = SLICER_AUDIO_ERROR;
        m_errMsg = "Audio is empty!";
        return {};
    }

    m_errCode = SlicerErrorCode::SLICER_OK;
    m_errMsg.clear();

    if ((frames + m_hopSize - 1) / m_hopSize <= m_minLength) {
        return {{ 0, frames }};
    }

    auto rms_list = get_rms(
            (channels > 1) ? multichannel_to_mono(waveform, channels) : waveform,
            m_winSize,
            m_hopSize
    );

    MarkerList sil_tags;
    std::size_t silence_start = 0;
    bool has_silence_start = false;
    std::size_t clip_start = 0;

    std::size_t pos = 0, pos_l = 0, pos_r = 0;

    for (std::size_t i = 0; i < rms_list.size(); i++) {
        double rms = rms_list[i];
        // Keep looping while frame is silent.
        if (rms < m_threshold) {
            // Record start of silent frames.
            if (!has_silence_start) {
                silence_start = i;
                has_silence_start = true;
            }
            continue;
        }
        // Keep looping while frame is not silent and silence start has not been recorded.
        if (!has_silence_start) {
            continue;
        }
        // Clear recorded silence start if interval is not enough or clip is too short
        bool is_leading_silence = ((silence_start == 0) && (i > m_maxSilKept));
        bool need_slice_middle = (
                ( (i - silence_start) >= m_minInterval) &&
                ( (i - clip_start) >= m_minLength) );
        if ((!is_leading_silence) && (!need_slice_middle)) {
            has_silence_start = false;
            continue;
        }

        // Need slicing. Record the range of silent frames to be removed.
        if ((i - silence_start) <= m_maxSilKept) {
            pos = argmin_range_view<double>(rms_list, silence_start, i + 1) + silence_start;
            if (silence_start == 0) {
                sil_tags.emplace_back(0, pos);
            }
            else {
                sil_tags.emplace_back(pos, pos);
            }
            clip_start = pos;
        }
        else if ((i - silence_start) <= (m_maxSilKept * 2)) {
            pos = argmin_range_view<double>(rms_list, i - m_maxSilKept, silence_start + m_maxSilKept + 1);
            pos += i - m_maxSilKept;
            pos_l = argmin_range_view<double>(rms_list, silence_start, silence_start + m_maxSilKept + 1) + silence_start;
            pos_r = argmin_range_view<double>(rms_list, i - m_maxSilKept, i + 1) + i - m_maxSilKept;
            if (silence_start == 0) {
                clip_start = pos_r;
                sil_tags.emplace_back(0, clip_start);
            }
            else {
                clip_start = std::max(pos_r, pos);
                sil_tags.emplace_back(std::min(pos_l, pos), clip_start);
            }
        }
        else {
            pos_l = argmin_range_view<double>(rms_list, silence_start, silence_start + m_maxSilKept + 1) + silence_start;
            pos_r = argmin_range_view<double>(rms_list, i - m_maxSilKept, i + 1) + i - m_maxSilKept;
            if (silence_start == 0) {
                sil_tags.emplace_back(0, pos_r);
            }
            else {
                sil_tags.emplace_back(pos_l, pos_r);
            }
            clip_start = pos_r;
        }
        has_silence_start = false;
    }
    // Deal with trailing silence.
    auto total_frames = rms_list.size();
    if (has_silence_start && ((total_frames - silence_start) >= m_minInterval)) {
        auto silence_end = std::min(total_frames - 1, silence_start + m_maxSilKept);
        pos = argmin_range_view<double>(rms_list, silence_start, silence_end + 1) + silence_start;
        sil_tags.emplace_back(pos, total_frames + 1);
    }
    // Apply and return slices.
    if (sil_tags.empty()) {
        return {{ 0, frames }};
    }
    else {
        MarkerList chunks;
        std::size_t begin = 0, end = 0;
        auto s0 = sil_tags[0].first;
        if (s0 > 0) {
            begin = 0;
            end = s0;
            chunks.emplace_back(begin * m_hopSize, std::min(frames, end * m_hopSize));
        }
        for (auto i = 0; i < sil_tags.size() - 1; i++) {
            begin = sil_tags[i].second;
            end = sil_tags[i + 1].first;
            chunks.emplace_back(begin * m_hopSize, std::min(frames, end * m_hopSize));
        }
        if (sil_tags.back().second < total_frames) {
            begin = sil_tags.back().second;
            end = total_frames;
            chunks.emplace_back(begin * m_hopSize, std::min(frames, end * m_hopSize));
        }
        return chunks;
    }
}

SlicerErrorCode Slicer::getErrorCode() const {
    return m_errCode;
}

std::string Slicer::getErrorMsg() const {
    return m_errMsg;
}
