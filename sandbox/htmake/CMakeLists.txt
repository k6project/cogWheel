include(${CMAKE_UTILS})
get_source_files(${CMAKE_CURRENT_LIST_DIR})
add_executable(htmake ${SOURCE_FILES})
target_link_libraries(htmake fltk)
