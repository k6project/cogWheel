include(${CMAKE_UTILS})
get_source_files(${CMAKE_CURRENT_LIST_DIR})
add_executable(vktest MACOSX_BUNDLE ${SOURCE_FILES})
add_vulkan_sdk(vktest)
