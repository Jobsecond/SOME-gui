cmake_minimum_required(VERSION 3.17)

project(SOME-gui VERSION 0.1 LANGUAGES CXX)


set(CMAKE_AUTOUIC ON)

set(CMAKE_AUTOMOC ON)

set(CMAKE_AUTORCC ON)



set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_STANDARD_REQUIRED ON)



find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)

find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)


set(PROJECT_SOURCES
        main.cpp
        Widgets/MainWindow.cpp
        Widgets/MainWindow.h
        Worker.cpp
        Worker.h
        Slicer/Slicer.cpp
        Slicer/Slicer.h
        Slicer/Slicer-inl.h
        Widgets/FileSelectionWidget.cpp
        Widgets/FileSelectionWidget.h
        Inference/Inference.cpp
        Inference/Inference.h
        Inference/InferenceUtils.hpp
        Inference/SOMEInference.cpp
        Inference/SOMEInference.h
        Inference/NotesStruct.h
        Inference/ExecutionProviderOptions.h
        OrtLoader.cpp
        OrtLoader.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)

    qt_add_executable(${PROJECT_NAME}

        MANUAL_FINALIZATION

        ${PROJECT_SOURCES}

    )

else()

    add_executable(${PROJECT_NAME}

        ${PROJECT_SOURCES}

    )

endif()


target_link_libraries(${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)


set_target_properties(${PROJECT_NAME} PROPERTIES

    #MACOSX_BUNDLE_GUI_IDENTIFIER some.gui

    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}

    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}

    MACOSX_BUNDLE TRUE

    WIN32_EXECUTABLE TRUE

    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)


# Add current directory to include path
target_include_directories(${PROJECT_NAME} PRIVATE .)


include(ort.cmake)

# ONNX Runtime libraries

target_include_directories(${PROJECT_NAME} PRIVATE
        ${ONNXRUNTIME_INCLUDE_PATH}
)

target_link_directories(${PROJECT_NAME} PRIVATE
        ${ONNXRUNTIME_LIB_PATH}
)

if(ENABLE_DML)
    if(DEFINED DML_INCLUDE_PATH)
        target_include_directories(${PROJECT_NAME} PRIVATE
                ${DML_INCLUDE_PATH}
        )
    endif()
    if(DEFINED DML_LIB_PATH)
        target_link_directories(${PROJECT_NAME} PRIVATE
                ${DML_LIB_PATH}
        )
    endif()
    target_compile_definitions(${PROJECT_NAME} PRIVATE
            ONNXRUNTIME_ENABLE_DML
    )
endif()

if(ENABLE_CUDA)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
            ONNXRUNTIME_ENABLE_CUDA
    )
endif()

option(DYNAMIC_LOAD_ORT_LIB "Dynamically load ONNX Runtime shared library instead of linking to it" on)
if(DYNAMIC_LOAD_ORT_LIB)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
            ORT_API_MANUAL_INIT
    )
else()
    if(WIN32 AND MSVC)
        target_link_libraries(${PROJECT_NAME} PRIVATE
                "user32.lib" "gdi32.lib" "onnxruntime.lib")
    else()
        target_link_libraries(${PROJECT_NAME} PRIVATE
                "-lonnxruntime")
    endif()
endif()


copy_ort_dlls(${PROJECT_NAME})

# libsndfile
find_package(SndFile CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE SndFile::sndfile)

# Sample rate conversion library
option(ENABLE_LIBSAMPLERATE "Enable libsamplerate library" off)
option(ENABLE_R8BRAIN "Enable r8brain-free-src library" on)

# If both libsamplerate and r8brain are enabled,
# r8brain will be chosen, and libsamplerate will be ignored.
if(ENABLE_R8BRAIN)
    message("Use r8brain as resample library")
    set(R8BRAIN_PATH "../libs/r8bsrc")
    add_subdirectory("${R8BRAIN_PATH}" "${R8BRAIN_PATH}")
    target_link_libraries(${PROJECT_NAME} PRIVATE r8bsrc)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
            SOME_ENABLE_R8BRAIN
    )
elseif(ENABLE_LIBSAMPLERATE)
    message("Use libsamplerate as resample library")
    find_package(SampleRate CONFIG REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE SampleRate::samplerate)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
            SOME_ENABLE_SAMPLERATE
    )
endif()

set(MIDIFILE_PATH "../libs/midifile")
add_subdirectory("${MIDIFILE_PATH}" "${MIDIFILE_PATH}")
target_link_libraries(${PROJECT_NAME} PRIVATE midifile)


install(TARGETS SOME-gui

        BUNDLE DESTINATION ..

        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})



if(QT_VERSION_MAJOR EQUAL 6)

    qt_finalize_executable(SOME-gui)

endif()
