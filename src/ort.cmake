option(ENABLE_DML "Enable DirectML support" off)
option(ENABLE_CUDA "Enable CUDA support" off)

if(ENABLE_DML)
    message("-- ONNX Runtime DirectML is enabled")
endif()

if(ENABLE_CUDA)
    message("-- ONNX Runtime CUDA is enabled")
endif()

if(NOT DEFINED ONNXRUNTIME_INCLUDE_PATH)  # ONNX Runtime include path
    message("-- Notice: ONNXRUNTIME_INCLUDE_PATH is not set. The build system will use system default paths.")
else()
    message("-- ONNXRUNTIME_INCLUDE_PATH is set to \"${ONNXRUNTIME_INCLUDE_PATH}\"")
endif()

if(NOT DEFINED ONNXRUNTIME_LIB_PATH)  # ONNX Runtime library path
    message("-- Notice: ONNXRUNTIME_LIB_PATH is not set. The build system will use system default paths.")
else()
    message("-- ONNXRUNTIME_LIB_PATH is set to \"${ONNXRUNTIME_LIB_PATH}\"")
endif()


if(ENABLE_DML)
    if(NOT DEFINED DML_INCLUDE_PATH)  # DirectML include path
        message("-- Notice: DML_INCLUDE_PATH is not set. The build system will use system default paths.")
    else()
        message("-- DML_INCLUDE_PATH is set to \"${DML_INCLUDE_PATH}\"")
    endif()

    if(NOT DEFINED DML_LIB_PATH)  # DirectML library path
        message("-- Notice: DML_LIB_PATH is not set. The build system will use system default paths.")
    else()
        message("-- DML_LIB_PATH is set to \"${DML_LIB_PATH}\"")
    endif()
endif()


# Windows might have an onnxruntime.dll in the system directory so it's more robust to manually copy the dlls to
# the output dir. Define a function to do so. This is called from the cmake file in the subdirectories.
function(copy_ort_dlls target_name)
    if (MSVC)
        if(ONNXRUNTIME_LIB_PATH)
            file(GLOB ORT_DLLS ${ONNXRUNTIME_LIB_PATH}/*.dll)
            foreach(ORT_DLL ${ORT_DLLS})
                add_custom_command(TARGET ${target_name} POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy ${ORT_DLL}  $<TARGET_FILE_DIR:${target_name}>)
            endforeach()
        endif()
        if(ENABLE_DML AND DML_LIB_PATH)
            list(APPEND DML_DLLS "DirectML.dll")
            if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
                list(APPEND DML_DLLS "DirectML.Debug.dll")
            endif()
            foreach(DML_DLL ${DML_DLLS})
                add_custom_command(TARGET ${target_name} POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy ${DML_LIB_PATH}/${DML_DLL}  $<TARGET_FILE_DIR:${target_name}>)
            endforeach()
        endif()
    endif()
endfunction()
