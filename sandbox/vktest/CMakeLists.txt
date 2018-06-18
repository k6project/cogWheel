include(${CMAKE_UTILS})

get_source_files(${CMAKE_CURRENT_LIST_DIR})
add_executable(vktest MACOSX_BUNDLE ${SOURCE_FILES})
target_compile_definitions(vktest PRIVATE PROGRAM_NAME="VkTest" PROGRAM_VERSION=0)
target_include_directories(vktest PRIVATE "${CMAKE_CURRENT_LIST_DIR}/inc")
target_include_directories(vktest PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src")

if(APPLE)
    target_compile_definitions(vktest PRIVATE GLFW_EXPOSE_NATIVE_COCOA)
elseif(WIN32)
    target_compile_definitions(vktest PRIVATE GLFW_EXPOSE_NATIVE_WIN32)
endif()
update_inline_shaders(vktest "${CMAKE_CURRENT_LIST_DIR}/src/shaders")
target_link_libraries(vktest glfw)
target_link_libraries(vktest core)
target_link_libraries(vktest gfx)

add_vulkan_sdk(vktest FALSE)
