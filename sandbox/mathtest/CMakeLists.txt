include(${CMAKE_UTILS})

get_source_files(${CMAKE_CURRENT_LIST_DIR})

add_executable(mathtest ${SOURCE_FILES})

target_compile_definitions(mathtest PRIVATE PROGRAM_NAME="MathTest" PROGRAM_VERSION=0)
target_include_directories(mathtest PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src")
target_link_libraries(mathtest core)
