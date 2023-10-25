#ifndef SOME_GUI_STRUCT_NOTES_
#define SOME_GUI_STRUCT_NOTES_

#include <vector>
#include <QMetaType>

namespace some {
    struct Notes {
        std::vector<float> note_midi;
        std::vector<char> note_rest;
        std::vector<float> note_dur;
    };
}  // namespace some

Q_DECLARE_METATYPE(some::Notes)

#endif  // SOME_GUI_STRUCT_NOTES_