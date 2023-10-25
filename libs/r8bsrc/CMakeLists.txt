project(r8bsrc)

set(R8BRAIN_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/r8brain-free-src")
add_library(${PROJECT_NAME} STATIC "${R8BRAIN_SOURCE_DIR}/r8bbase.cpp")
target_include_directories(${PROJECT_NAME} PUBLIC ${R8BRAIN_SOURCE_DIR})
