#ifndef SOME_GUI_EXECUTIONPROVIDEROPTIONS_H
#define SOME_GUI_EXECUTIONPROVIDEROPTIONS_H

#include <QObject>
#include <QMetaType>

namespace some {
    enum class ExecutionProvider {
        CPU,
        CUDA,
        DirectML
    };  // enum class ExecutionProvider
}  // namespace some

Q_DECLARE_METATYPE(some::ExecutionProvider)

#endif //SOME_GUI_EXECUTIONPROVIDEROPTIONS_H
