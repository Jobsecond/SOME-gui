
project(midifile C CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(MIDIFILE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/midifile")
set(MIDIFILE_SRC_DIR "${MIDIFILE_DIR}/src")
set(MIDIFILE_INCLUDE_DIR "${MIDIFILE_DIR}/include")

set(SRCS
        #${MIDIFILE_SRC_DIR}/Options.cpp
        ${MIDIFILE_SRC_DIR}/Binasc.cpp
        ${MIDIFILE_SRC_DIR}/MidiEvent.cpp
        ${MIDIFILE_SRC_DIR}/MidiEventList.cpp
        ${MIDIFILE_SRC_DIR}/MidiFile.cpp
        ${MIDIFILE_SRC_DIR}/MidiMessage.cpp
)

set(HDRS
        ${MIDIFILE_INCLUDE_DIR}/Binasc.h
        ${MIDIFILE_INCLUDE_DIR}/MidiEvent.h
        ${MIDIFILE_INCLUDE_DIR}/MidiEventList.h
        ${MIDIFILE_INCLUDE_DIR}/MidiFile.h
        ${MIDIFILE_INCLUDE_DIR}/MidiMessage.h
        ${MIDIFILE_INCLUDE_DIR}/Options.h
)

add_library(${PROJECT_NAME} STATIC ${SRCS} ${HDRS})

target_include_directories(${PROJECT_NAME} PRIVATE
        "${MIDIFILE_INCLUDE_DIR}"
)

target_include_directories(${PROJECT_NAME} PUBLIC
        "$<BUILD_INTERFACE:${MIDIFILE_INCLUDE_DIR}>"
)
