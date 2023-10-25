#ifndef SOME_GUI_SOMEINFERENCE_H
#define SOME_GUI_SOMEINFERENCE_H

#include <cstdint>

#include <QObject>
#include <QString>
#include <QVector>

#include "Inference.h"
#include "NotesStruct.h"

namespace some {

    class SOMEInference : public Inference {
        Q_OBJECT
        Q_PROPERTY(bool supportBatch READ supportBatch)
    public:
        explicit SOMEInference(const QString &modelPath, QObject *parent = nullptr);
        Notes infer(const std::vector<float> &waveform);
        Notes infer(const std::vector<float> &waveform, size_t begin, size_t count);
        bool supportBatch() const;
    protected:
        bool postInitCheck() override;
        void postCleanup() override;
    private:
        bool m_supportBatch;
    };

} // namespace some

#endif //SOME_GUI_SOMEINFERENCE_H
