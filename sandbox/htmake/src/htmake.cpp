#define _CRT_SECURE_NO_WARNINGS

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdbool.h>
#include <core/memory.h>

#ifdef WIN32
#ifdef _MBCS
#define UTF8(s) s
#else
#error "Only multibyte strings are supported on Windows"
#endif
#include <direct.h>
#define getcwd _getcwd
#define chdir _chdir
#else
#include <unistd.h>
#endif

/* Max. string length: 128k */
#define HTM_STRING_MAX (0xFFFFu << 1)

/* Memory budget: ~4 max strings */
#define HTM_ALLOC_SIZE (HTM_STRING_MAX << 2)

typedef struct htmContext_t
{
	memStackAlloc_t* memory;
	const char* header;
	const char* footer;
} htmContext_t;

/*
using namespace std;
const string& htmEscape(const string& str)
{
	thread_local static string result;
    using pattern_t = pair<string, string>;
    static const vector<pattern_t> HTML_ESCAPE_CHARS
    {
        { UTF8("&"), UTF8("&amp;") },  
		{ UTF8("ü"), UTF8("&uuml;")}, 
		{ UTF8("Ü"), UTF8("&Uuml;")}, 
		{ UTF8("ä"), UTF8("&auml;")}, 
		{ UTF8("Ä"), UTF8("&Auml;")},
		{ UTF8("ö"), UTF8("&ouml;")}, 
		{ UTF8("Ö"), UTF8("&Ouml;")},
		{ UTF8(">"), UTF8("&gt;")  }, 
        { UTF8("<"), UTF8("&lt;")  },   
    };
	result.clear();
    result.reserve(str.size() << 1);
    for (auto it = str.begin(); it != str.end();)
    {
        size_t escaped = 0;
        const char* start = &*it;
        for (const auto& entry : HTML_ESCAPE_CHARS)
        {
            const string& pattern = entry.first;
            if ( !pattern.compare(0, pattern.size(), start, pattern.size()) )
            {
                result.append(entry.second);
                escaped = pattern.size();
                break;
            }
        }
        if (!escaped)
        {
            result.append(1, *it++);
        }
        else
        {
            it += escaped;
        }
    }
    return result;
}
*/

const char* htmLoadFile(const char* path, memStackAlloc_t* memory)
{
	char* result = NULL;
	FILE* fp = fopen(path, "rb");
	if (fp)
	{
		fseek(fp, 0, SEEK_END);
		long pos = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		result = memStackAlloc(memory, pos + 1);
		if (fread(result, pos, 1, fp) < 1)
		{
			printf("Failed to read file: %s\n", path);
			memStackFree(memory, result);
			result = NULL;
		}
		else
		{
			result[pos] = 0;
		}
		fclose(fp);
	}
	else
	{
		printf("Failed to open file: %s\n", path);
	}
	return result;
}

void htmInitContext(htmContext_t* ctx, int argc, const char* argv[])
{
	char activeOption = 0;
	for (int i = 0; i < argc; i++)
	{
		if (*argv[i] == '-' && !activeOption)
		{
			assert(isalpha(argv[i][1]));
			activeOption = toupper(argv[i][1]);
			continue;
		}
		switch (activeOption)
		{
		case 'D':
			if (chdir(argv[i]) == 0)
			{
				printf("Working directory: %s\n", argv[i]);
				ctx->header = htmLoadFile("header.htpl", ctx->memory);
				ctx->footer = htmLoadFile("footer.htpl", ctx->memory);
			}
			else
			{
				printf("Failed to set working directory: %s\n", argv[i]);
			}
			break;
		case 'I':
			printf("Processing template: %s\n", argv[i]);
			break;
		default:
			break;
		}
	}
}

int main(int argc, const char * argv[])
{
	htmContext_t ctx = { .memory = NULL };
	memStackInit(&ctx.memory, HTM_ALLOC_SIZE);
	htmInitContext(&ctx, argc, argv);
	//analyze arguments
	memStackDestroy(&ctx.memory);
    return 0;
}
