#include <unordered_map>
#include <QDebug>

#ifdef ONNXRUNTIME_ENABLE_DML
#include <dml_provider_factory.h>
#endif

#include "Inference.h"

namespace some {

    Inference::Inference(const QString &modelPath, QObject *parent)
            : QObject(parent),
              m_modelPath(modelPath),
              m_env(ORT_LOGGING_LEVEL_WARNING, "SOME"),
              m_session(nullptr),
              ortApi(Ort::GetApi()) {}

    QString Inference::getModelPath() {
        return m_modelPath;
    }

    bool Inference::initSession(ExecutionProvider ep, int deviceIndex) {
        try {
            auto options = Ort::SessionOptions();
            switch (ep) {
                case ExecutionProvider::DirectML:
#ifdef ONNXRUNTIME_ENABLE_DML
                {
                    Q_EMIT logMsgInfo("Try DirectML...");
                    const OrtDmlApi *ortDmlApi;
                    auto getApiStatus = Ort::Status(ortApi.GetExecutionProviderApi("DML", ORT_API_VERSION, reinterpret_cast<const void **>(&ortDmlApi)));
                    if (getApiStatus.IsOK()) {
                        Q_EMIT logMsgInfo("Successfully got DirectML API.");
                        options.DisableMemPattern();
                        options.SetExecutionMode(ExecutionMode::ORT_SEQUENTIAL);

                        auto status = Ort::Status(ortDmlApi->SessionOptionsAppendExecutionProvider_DML(options, deviceIndex));
                        if (status.IsOK()) {
                            Q_EMIT logMsgInfo("Successfully appended DirectML Execution Provider.");
                        }
                        else {
                            Q_EMIT logMsgError(
                                    QString(
                                            "Failed to append DirectML Execution Provider. Use CPU instead. "
                                            "Error code: %1, Reason: %2")
                                    .arg(status.GetErrorCode())
                                    .arg(QString::fromStdString(status.GetErrorMessage())));
                        }
                    }
                    else {
                        Q_EMIT logMsgError(
                                QString("Failed to get DirectML API. Reason: ") +
                                QString::fromStdString(getApiStatus.GetErrorMessage()) +
                                QString(" Now use CPU instead."));
                    }
                }
#else
                    Q_EMIT logMsgInfo("The software is not built with DirectML support. Use CPU instead.");
#endif
                    break;
                case ExecutionProvider::CUDA:
#ifdef ONNXRUNTIME_ENABLE_CUDA
                {
                    Q_EMIT logMsgInfo("Try CUDA...");
                    OrtCUDAProviderOptionsV2 *cudaOptions = nullptr;
                    ortApi.CreateCUDAProviderOptions(&cudaOptions);

                    // The following block of code sets device_id
                    {
                        // Device ID from int to string
                        auto cudaDeviceIdStr = std::to_string(deviceIndex);
                        auto cudaDeviceIdStr_cstyle = cudaDeviceIdStr.c_str();

                        constexpr int CUDA_OPTIONS_SIZE = 2;
                        const char *cudaOptionsKeys[CUDA_OPTIONS_SIZE] = { "device_id", "cudnn_conv_algo_search" };
                        const char *cudaOptionsValues[CUDA_OPTIONS_SIZE] = { cudaDeviceIdStr_cstyle, "DEFAULT" };
                        auto updateStatus = Ort::Status(
                                ortApi.UpdateCUDAProviderOptions(cudaOptions, cudaOptionsKeys, cudaOptionsValues,
                                                                 CUDA_OPTIONS_SIZE));
                        if (!updateStatus.IsOK()) {
                            Q_EMIT logMsgError(
                                    QString(
                                            "Failed to update CUDA Execution Provider options. Use CPU instead. "
                                            "Error code: %1, Reason: %2")
                                            .arg(updateStatus.GetErrorCode())
                                            .arg(QString::fromStdString(updateStatus.GetErrorMessage())));
                        }
                    }

                    auto status = Ort::Status(
                            ortApi.SessionOptionsAppendExecutionProvider_CUDA_V2(options, cudaOptions));
                    ortApi.ReleaseCUDAProviderOptions(cudaOptions);

                    if (status.IsOK()) {
                        Q_EMIT logMsgInfo("Successfully appended CUDA Execution Provider.");
                    }
                    else {
                        Q_EMIT logMsgError(
                                QString(
                                        "Failed to append CUDA Execution Provider. Use CPU instead. "
                                        "Error code: %1, Reason: %2")
                                        .arg(status.GetErrorCode())
                                        .arg(QString::fromStdString(status.GetErrorMessage())));
                    }
                }
#else
                    Q_EMIT logMsgInfo("The software is not built with CUDA support. Use CPU instead.");
#endif
                    break;
                default:
                    // CPU and other
                    Q_EMIT logMsgInfo("Use CPU.");
                    break;
            }

            m_session = Ort::Session(
                    m_env,
                    m_modelPath
#ifdef _WIN32
                    .toStdWString()
#else
                    .toStdString()
#endif
                    .c_str(), options);

            return postInitCheck();
        }
        catch (const Ort::Exception &ortException) {
            Q_EMIT logMsgError(QString("[ONNXRuntimeError] : %1 : %2")
                                     .arg(ortException.GetOrtErrorCode())
                                     .arg(ortException.what()));
        }
        return false;
    }


    bool Inference::hasSession() {
        return m_session;
    }

    void Inference::endSession() {
        {
            Ort::Session emptySession(nullptr);
            std::swap(m_session, emptySession);
        }
        postCleanup();
    }

    bool Inference::postInitCheck() {
        return true;
    }

    void Inference::postCleanup() {}

}  // namespace some
