cmake_minimum_required(VERSION 3.1.3)
project(cogWheel C CXX)

add_definitions(-DENGINE_NAME="cogWheel" -DENGINE_VERSION=0x000001)
if(WIN32)
    add_definitions(-DWIN32_LEAN_AND_MEAN -DPLATFORM_WIN32)
elseif(APPLE)
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++14")
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
    add_definitions(-DPLATFORM_MACOS)
endif()

set(WORKSPACE_ROOT ${CMAKE_CURRENT_LIST_DIR})
set(CMAKE_UTILS ${CMAKE_CURRENT_LIST_DIR}/CMakeUtils.txt)

add_subdirectory(extern/common/shaderc)
add_subdirectory(extern/common/fltk)

add_subdirectory(engine)
#add_subdirectory(tools)
add_subdirectory(sandbox)
