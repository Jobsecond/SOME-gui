
#ifndef SOME_GUI_INFERENCE_H
#define SOME_GUI_INFERENCE_H

#include <string>
#include <vector>
#include <QObject>
#include <QString>

#include <onnxruntime_cxx_api.h>

#include "ExecutionProviderOptions.h"

namespace some {

    class Inference : public QObject {
        Q_OBJECT
    public:
        explicit Inference(const QString &modelPath, QObject *parent = nullptr);

        bool initSession(ExecutionProvider ep = ExecutionProvider::CPU, int deviceIndex = 0);

        void endSession();

        bool hasSession();

        QString getModelPath();

    Q_SIGNALS:
        void logMsgInfo(const QString &msg);
        void logMsgError(const QString &msg);

    protected:
        QString m_modelPath;

        // Ort::Env must be initialized before Ort::Session.
        // (In this class, it should be defined before Ort::Session)
        // Otherwise, access violation will occur when Ort::Session destructor is called.
        Ort::Env m_env;
        Ort::Session m_session;
        OrtApi const &ortApi; // Uses ORT_API_VERSION
    protected:
        virtual bool postInitCheck();

        virtual void postCleanup();
    };  // class Inference

}  // namespace some

#endif // SOME_GUI_INFERENCE_H
