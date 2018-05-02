#include "ui/shaderz.h"
#include "callbacks.h"

#include <assert.h>
#include <string.h>

#include <shaderc/shaderc.h>

void compileShader(compileJob_t* job)
{
    shaderc_compiler_t compiler = NULL;
    assert(compiler = shaderc_compiler_initialize());
    shaderc_compile_options_t options = NULL;
    assert(options = shaderc_compile_options_initialize());
    shaderc_compile_options_set_source_language(options, job->lang);
    shaderc_compilation_result_t result = NULL;
    assert(result = shaderc_compile_into_spv_assembly(compiler,
        job->code, job->length,
        job->type, job->fileName, job->mainProc, options));
    if (shaderc_result_get_compilation_status(result) == shaderc_compilation_status_success)
    {
        job->code = shaderc_result_get_bytes(result);
        job->length = shaderc_result_get_length(result);
        job->onSuccess(job);
    }
	else
	{
		job->code = shaderc_result_get_error_message(result);
		job->length = strlen(job->code);
		job->onError(job);
	}
    shaderc_compiler_release(compiler);
}

int main(int argc, char** argv)
{
	return shaderzShowGui(argc, argv);
}
