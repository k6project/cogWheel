#include <vector>
#include <cstdio>
#include <cstdlib>
#include <ShaderLang.h>
#include <SPIRV/GLSL.std.450.h>
#include <SPIRV/GlslangToSpv.h>
#include <SPIRV/disassemble.h>

using namespace std;
using namespace glslang;

static TBuiltInResource defaults
{
    /* .MaxLights = */ 32,
    /* .MaxClipPlanes = */ 6,
    /* .MaxTextureUnits = */ 32,
    /* .MaxTextureCoords = */ 32,
    /* .MaxVertexAttribs = */ 64,
    /* .MaxVertexUniformComponents = */ 4096,
    /* .MaxVaryingFloats = */ 64,
    /* .MaxVertexTextureImageUnits = */ 32,
    /* .MaxCombinedTextureImageUnits = */ 80,
    /* .MaxTextureImageUnits = */ 32,
    /* .MaxFragmentUniformComponents = */ 4096,
    /* .MaxDrawBuffers = */ 32,
    /* .MaxVertexUniformVectors = */ 128,
    /* .MaxVaryingVectors = */ 8,
    /* .MaxFragmentUniformVectors = */ 16,
    /* .MaxVertexOutputVectors = */ 16,
    /* .MaxFragmentInputVectors = */ 15,
    /* .MinProgramTexelOffset = */ -8,
    /* .MaxProgramTexelOffset = */ 7,
    /* .MaxClipDistances = */ 8,
    /* .MaxComputeWorkGroupCountX = */ 65535,
    /* .MaxComputeWorkGroupCountY = */ 65535,
    /* .MaxComputeWorkGroupCountZ = */ 65535,
    /* .MaxComputeWorkGroupSizeX = */ 1024,
    /* .MaxComputeWorkGroupSizeY = */ 1024,
    /* .MaxComputeWorkGroupSizeZ = */ 64,
    /* .MaxComputeUniformComponents = */ 1024,
    /* .MaxComputeTextureImageUnits = */ 16,
    /* .MaxComputeImageUniforms = */ 8,
    /* .MaxComputeAtomicCounters = */ 8,
    /* .MaxComputeAtomicCounterBuffers = */ 1,
    /* .MaxVaryingComponents = */ 60,
    /* .MaxVertexOutputComponents = */ 64,
    /* .MaxGeometryInputComponents = */ 64,
    /* .MaxGeometryOutputComponents = */ 128,
    /* .MaxFragmentInputComponents = */ 128,
    /* .MaxImageUnits = */ 8,
    /* .MaxCombinedImageUnitsAndFragmentOutputs = */ 8,
    /* .MaxCombinedShaderOutputResources = */ 8,
    /* .MaxImageSamples = */ 0,
    /* .MaxVertexImageUniforms = */ 0,
    /* .MaxTessControlImageUniforms = */ 0,
    /* .MaxTessEvaluationImageUniforms = */ 0,
    /* .MaxGeometryImageUniforms = */ 0,
    /* .MaxFragmentImageUniforms = */ 8,
    /* .MaxCombinedImageUniforms = */ 8,
    /* .MaxGeometryTextureImageUnits = */ 16,
    /* .MaxGeometryOutputVertices = */ 256,
    /* .MaxGeometryTotalOutputComponents = */ 1024,
    /* .MaxGeometryUniformComponents = */ 1024,
    /* .MaxGeometryVaryingComponents = */ 64,
    /* .MaxTessControlInputComponents = */ 128,
    /* .MaxTessControlOutputComponents = */ 128,
    /* .MaxTessControlTextureImageUnits = */ 16,
    /* .MaxTessControlUniformComponents = */ 1024,
    /* .MaxTessControlTotalOutputComponents = */ 4096,
    /* .MaxTessEvaluationInputComponents = */ 128,
    /* .MaxTessEvaluationOutputComponents = */ 128,
    /* .MaxTessEvaluationTextureImageUnits = */ 16,
    /* .MaxTessEvaluationUniformComponents = */ 1024,
    /* .MaxTessPatchComponents = */ 120,
    /* .MaxPatchVertices = */ 32,
    /* .MaxTessGenLevel = */ 64,
    /* .MaxViewports = */ 16,
    /* .MaxVertexAtomicCounters = */ 0,
    /* .MaxTessControlAtomicCounters = */ 0,
    /* .MaxTessEvaluationAtomicCounters = */ 0,
    /* .MaxGeometryAtomicCounters = */ 0,
    /* .MaxFragmentAtomicCounters = */ 8,
    /* .MaxCombinedAtomicCounters = */ 8,
    /* .MaxAtomicCounterBindings = */ 1,
    /* .MaxVertexAtomicCounterBuffers = */ 0,
    /* .MaxTessControlAtomicCounterBuffers = */ 0,
    /* .MaxTessEvaluationAtomicCounterBuffers = */ 0,
    /* .MaxGeometryAtomicCounterBuffers = */ 0,
    /* .MaxFragmentAtomicCounterBuffers = */ 1,
    /* .MaxCombinedAtomicCounterBuffers = */ 1,
    /* .MaxAtomicCounterBufferSize = */ 16384,
    /* .MaxTransformFeedbackBuffers = */ 4,
    /* .MaxTransformFeedbackInterleavedComponents = */ 64,
    /* .MaxCullDistances = */ 8,
    /* .MaxCombinedClipAndCullDistances = */ 8,
    /* .MaxSamples = */ 4,
    /* .limits = */ {
        /* .nonInductiveForLoops = */ 1,
        /* .whileLoops = */ 1,
        /* .doWhileLoops = */ 1,
        /* .generalUniformIndexing = */ 1,
        /* .generalAttributeMatrixVectorIndexing = */ 1,
        /* .generalVaryingIndexing = */ 1,
        /* .generalSamplerIndexing = */ 1,
        /* .generalVariableIndexing = */ 1,
        /* .generalConstantMatrixVectorIndexing = */ 1,
    }
};

bool loadShaderFile(const char* path, char*& data, int& size)
{
	FILE* fp = nullptr; 
    bool result = false;
#ifdef _MSC_VER
	fopen_s(&fp, path, "rb");
#elif
	fp = fopen_s(path, "rb");
#endif
    if (fp)
    {
        fseek(fp, 0, SEEK_END);
        size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        char* buff = (char*)calloc(1, size + 1);
        if (fread(buff, size, 1, fp) == 1)
        {
            data = buff;
            result = true;
        }
        else
        {
            free(buff);
        }
        fclose(fp);
    }
    return result;
}

int main(int argc, const char * argv[])
{
    int shaderSize = 0;
    char* shaderData = nullptr;
    if (argc > 0 && loadShaderFile(argv[1], shaderData, shaderSize))
    {
        if (InitializeProcess())
        {
            TShader vert(EShLangVertex);
            TShader frag(EShLangFragment);
			TShader::ForbidIncluder includer;
            vert.setEntryPoint("main");
            vert.setPreamble("#define _VS_\n");
            vert.setStringsWithLengths(&shaderData, &shaderSize, 1);
			vert.setEnvInput(EShSourceGlsl, EShLangVertex, EShClientVulkan, EShTargetVulkan_1_0);
			frag.setEntryPoint("main");
			frag.setPreamble("#define _FS_\n");
			frag.setStringsWithLengths(&shaderData, &shaderSize, 1);
			frag.setEnvInput(EShSourceGlsl, EShLangFragment, EShClientVulkan, EShTargetVulkan_1_0);
            if (!vert.parse(&defaults, 450, ECoreProfile, false, true, EShMsgDefault, includer))
            {
                printf("%s\n", vert.getInfoLog());
            }
			else 
			{
				if (!frag.parse(&defaults, 450, ECoreProfile, false, true, EShMsgDefault, includer))
				{
					printf("%s\n", frag.getInfoLog());
				}
				else
				{
					TProgram program;
					program.addShader(&vert);
					program.addShader(&frag);
					if (!program.link(EShMsgDefault))
					{
						printf("%s\n", program.getInfoLog());
					}
					else
					{
						program.buildReflection();
						vector<unsigned int> spvBuffer;
						for (int i = 0; i < EShLangCount; i++)
						{
							EShLanguage stage = static_cast<EShLanguage>(i);
							if (TIntermediate* tmp = program.getIntermediate(stage))
							{
								SpvOptions spvOptions;
								spv::SpvBuildLogger logger;
								spvOptions.optimizeSize = false;
								spvOptions.generateDebugInfo = true;
								spvOptions.disableOptimizer = true;
								GlslangToSpv(*tmp, spvBuffer, &logger, &spvOptions);
								//glslang::OutputSpvBin(spvBuffer, "filename.xxx");
								//write words as sequences of 4 bytes, without order adjustments
								//or glslang::OutputSpvHex
								//spv::Disassemble(cout, spvBuffer);
							}
						}
					}
				}
			}
            FinalizeProcess();
        }
        delete shaderData;
        shaderSize = 0;
    }
    return 0;
}
