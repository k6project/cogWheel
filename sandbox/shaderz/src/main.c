#include "ui/shaderz.h"

#include <assert.h>

#include <shaderc/shaderc.h>

int main(int argc, char** argv)
{
    shaderc_compiler_t compiler = NULL;
    assert(compiler = shaderc_compiler_initialize());
    shaderc_compile_options_t options = NULL;
    assert(options = shaderc_compile_options_initialize());
    
    shaderc_compile_options_set_source_language(options, shaderc_source_language_glsl);
    
    shaderc_compiler_release(compiler);
    
	return shaderzShowGui(argc, argv);
}
