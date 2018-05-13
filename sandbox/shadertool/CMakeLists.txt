include(${CMAKE_UTILS})
get_source_files(${CMAKE_CURRENT_LIST_DIR})
add_executable(shadertool ${SOURCE_FILES})
target_link_libraries(shadertool 
    glslang
    SPIRV-Tools
    SPIRV-Tools-opt 
    SPIRV
    OGLCompiler
    OSDependent
    HLSL
)
