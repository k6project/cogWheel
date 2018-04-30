include(${CMAKE_UTILS})
get_source_files(${CMAKE_CURRENT_LIST_DIR})
add_executable(shaderz ${SOURCE_FILES})
target_link_libraries(shaderz fltk)
