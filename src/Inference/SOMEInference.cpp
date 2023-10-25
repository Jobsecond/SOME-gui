#include <limits>

#include "SOMEInference.h"
#include "InferenceUtils.hpp"

namespace some {
    SOMEInference::SOMEInference(const QString &modelPath, QObject *parent)
            : Inference(modelPath, parent), m_supportBatch(false) {}

    Notes SOMEInference::infer(const std::vector<float> &waveform) {
        return infer(waveform, static_cast<size_t>(0), waveform.size());
    }

    Notes SOMEInference::infer(const std::vector<float> &waveform, size_t begin, size_t count) {
        if (!m_session) {
            logMsgError("Session is not initialized!");
            return {};
        }

        if (begin >= waveform.size()) {
            return {};
        }

        constexpr auto kInt64Max = std::numeric_limits<int64_t>::max();
        constexpr auto kSizeTypeMax = std::numeric_limits<size_t>::max();

        // Avoid overflow
        if (count > kSizeTypeMax - begin) {
            count = kSizeTypeMax - begin;
        }

        // Avoid index out of range
        if (begin + count > waveform.size()) {
            count = waveform.size() - begin;
        }

        if (count == 0) {
            return {};
        }

        std::vector<const char *> inputNames;
        std::vector<Ort::Value> inputTensors;

        Ort::AllocatorWithDefaultOptions allocator;

        // waveform

        {
            if (count > kInt64Max) {
                count = kInt64Max;
            }
            std::vector<int64_t> inputShape = {1, static_cast<int64_t>(count)};
            inputTensors.emplace_back(Ort::Value::CreateTensor<float>(
                    allocator.GetInfo(),
                    const_cast<float *>(waveform.data() + begin),
                    count,
                    inputShape.data(),
                    inputShape.size()
            ));
            inputNames.emplace_back("waveform");
        }

        // Create output names
        std::vector<const char *> outputNames = { "note_midi", "note_rest", "note_dur" };

        try {
            // Run the session
            auto outputTensors = m_session.Run(
                    Ort::RunOptions{},
                    inputNames.data(),
                    inputTensors.data(),
                    inputNames.size(),
                    outputNames.data(),
                    outputNames.size());

            // Get the output tensor
            Ort::Value &note_midi = outputTensors[0];
            Ort::Value &note_rest = outputTensors[1];
            Ort::Value &note_dur = outputTensors[2];
            Notes notes;
            {
                auto dataPtr = note_midi.GetTensorData<float>();
                notes.note_midi = std::vector<float>(dataPtr, dataPtr + note_midi.GetTensorTypeAndShapeInfo().GetElementCount());
            }
            {
                auto dataPtr = note_rest.GetTensorData<bool>();
                notes.note_rest = std::vector<char>(dataPtr, dataPtr + note_rest.GetTensorTypeAndShapeInfo().GetElementCount());
            }
            {
                auto dataPtr = note_dur.GetTensorData<float>();
                notes.note_dur = std::vector<float>(dataPtr, dataPtr + note_dur.GetTensorTypeAndShapeInfo().GetElementCount());
            }
            return notes;
        }
        catch (const Ort::Exception &ortException) {
            Q_EMIT logMsgError(QString("[ONNXRuntimeError] : %1 : %2")
                                       .arg(ortException.GetOrtErrorCode())
                                       .arg(ortException.what()));
        }
        return {};
    }

    bool SOMEInference::postInitCheck() {
        if (!m_session) {
            return false;
        }
        auto inputCount = m_session.GetInputCount();
        size_t requiredInputCount = 1;
        if (inputCount != requiredInputCount) {
            endSession();
            logMsgError(QString("Invalid model! The input count should be %1, but the model has %2.")
                    .arg(requiredInputCount).arg(inputCount));
            return false;
        }
        auto inputNames = getSupportedInputNames(m_session);
        if (!hasKey(inputNames, "waveform")) {
            endSession();
            logMsgError("Invalid model! The model should contain input name `waveform`.");
            return false;
        }
        auto inputShape = m_session.GetInputTypeInfo(0).GetTensorTypeAndShapeInfo().GetShape();
        if (inputShape.size() != 2 && inputShape[0] != 1 && inputShape[0] != -1 && inputShape[1] != -1) {
            endSession();
            QString errMsg = "Invalid model! The input shape should be 2 dimensions. "
                             "The first dimension should be 1 or dynamic, "
                             "and the last dimension should be dynamic. "
                             "Actual shape: [";
            for (size_t shapeIndex = 0; shapeIndex < inputShape.size(); ++shapeIndex) {
                errMsg += QString::number(inputShape[shapeIndex]);
                if (shapeIndex < inputShape.size() - 1) {
                    errMsg += ", ";
                }
            }
            errMsg += ']';
            logMsgError(errMsg);
            return false;
        }
        m_supportBatch = (inputShape[0] == -1);
        return true;
    }


    void SOMEInference::postCleanup() {
        m_supportBatch = false;
    }

    bool SOMEInference::supportBatch() const {
        return m_supportBatch;
    }
} // namespace some