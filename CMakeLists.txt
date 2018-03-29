cmake_minimum_required(VERSION 3.1.3)
project(cogWheel C CXX)
set(COGWHEEL_ROOT ${CMAKE_CURRENT_LIST_DIR})
set(CMAKE_UTILS ${CMAKE_CURRENT_LIST_DIR}/CMakeUtils.txt)

#add_subdirectory(engine)
#add_subdirectory(tools)
add_subdirectory(sandbox)
