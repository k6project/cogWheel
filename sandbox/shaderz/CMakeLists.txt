include(${CMAKE_UTILS})
get_source_files(${CMAKE_CURRENT_LIST_DIR})
add_executable(shaderz ${SOURCE_FILES})
target_link_libraries(shaderz 
    fltk 
    shaderc 
    shaderc_util 
    glslang
    SPIRV-Tools
    SPIRV-Tools-opt 
    SPIRV
    OGLCompiler
    OSDependent
    HLSL
)
if(APPLE)
    find_library(core_foundation CoreFoundation)
    find_library(core_graphics CoreGraphics)
    find_library(cocoa Cocoa)
    mark_as_advanced(
        core_foundation
        core_graphics
        cocoa
    )
    target_link_libraries(shaderz
        ${core_foundation}
        ${core_graphics}
        ${cocoa}
    )
endif()
