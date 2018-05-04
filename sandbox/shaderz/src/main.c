#include "ui/shaderz.h"
#include "callbacks.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include <shaderc/shaderc.h>

void compileShader(compileJob_t* job)
{
    shaderc_compiler_t compiler = NULL;
    assert(compiler = shaderc_compiler_initialize());
    shaderc_compile_options_t options = NULL;
    assert(options = shaderc_compile_options_initialize());
    shaderc_compile_options_set_source_language(options, job->lang);
    shaderc_compilation_result_t result = NULL;
    switch (job->jobType)
    {
        case COMPILE_JOB_SPIRV_ASM:
            assert(result = shaderc_compile_into_spv_assembly(compiler,
                job->code, job->length, job->type, job->fileName, job->mainProc, options));
            break;
        case COMPILE_JOB_SPIRV_CPP:
        case COMPILE_JOB_SPIRV_BIN:
            assert(result = shaderc_compile_into_spv(compiler,
                job->code, job->length, job->type, job->fileName, job->mainProc, options));
            break;
        default:
            break;
    }
    if (shaderc_result_get_compilation_status(result) == shaderc_compilation_status_success)
    {
        job->code = shaderc_result_get_bytes(result);
        job->length = shaderc_result_get_length(result);
        if (job->jobType == COMPILE_JOB_SPIRV_CPP)
        {
            char* code = (char*)calloc(job->length << 3, sizeof(char));
            char* pos = code + sprintf(code, "{\n");
            for (size_t i = 0; i < job->length; i++)
            {
                if ((i % 10) == 0)
                {
                    pos += sprintf(pos, "    ");
                }
                pos += sprintf(pos, "0x%02x", (unsigned char)job->code[i]);
                if (i < job->length - 1)
                {
                    *pos++ = ',';
                    *pos++ = ((i % 10) == 9) ? '\n' : ' ';
                }
                else
                {
                    *pos++ = ',';
                    *pos++ = '\n';
                }
            }
            pos += sprintf(pos, "}\n");
            job->code = code;
            job->length = pos - code;
        }
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
