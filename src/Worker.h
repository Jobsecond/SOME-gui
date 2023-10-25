#ifndef SOME_GUI_WORKER_H
#define SOME_GUI_WORKER_H

#include <QThread>

#include "Inference/ExecutionProviderOptions.h"

class QString;
class QColor;

class Worker : public QThread {
    Q_OBJECT
public:
    explicit Worker(QObject *parent = nullptr);
    explicit Worker(const QString &modelPath,
                    const QString &audioPath,
                    double tempo,
                    const QString &outPath,
                    some::ExecutionProvider ep,
                    int deviceIndex,
                    int batchSize = 1,
                    QObject *parent = nullptr);
Q_SIGNALS:
    void logMsgInfo(const QString &msg);
    void logMsgError(const QString &msg);
    void logMsgWithColor(const QString &msg, const QColor &color);

protected:
    void run() override;

private:
    QString m_modelPath;
    QString m_audioPath;
    double m_tempo;
    QString m_outPath;
    int m_deviceIndex;
    int m_batchSize;
    some::ExecutionProvider m_ep = some::ExecutionProvider::CPU;
};


#endif //SOME_GUI_WORKER_H
