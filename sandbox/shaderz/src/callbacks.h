#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>
#include <shaderc/shaderc.h>
    
enum
{
    COMPILE_JOB_SPIRV_BIN,
    COMPILE_JOB_SPIRV_ASM,
    COMPILE_JOB_SPIRV_CPP,
    COMPILE_JOB_INVALID,
};

struct compileJob_t;
    
typedef void (*compileCallback_t)(const struct compileJob_t*);
  
typedef struct compileJob_t
{
    const char* code;
    size_t length;
    unsigned int jobType         : 32;
    shaderc_source_language lang : 16;
    shaderc_shader_kind type     : 16;
    const char* fileName;
    const char* mainProc;
    compileCallback_t onSuccess;
    compileCallback_t onError;
    void* context;
} compileJob_t;
    
void compileShader(compileJob_t* job);
    
#ifdef __cplusplus
}
#endif
