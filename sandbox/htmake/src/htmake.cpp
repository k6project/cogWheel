#include <string>
#include <vector>
#include <iostream>
#include <FL/fl_ask.H>

#ifdef WIN32
#ifdef _MBCS
#define UTF8(s) s
#else
#error "Only multibyte strings are supported on Windows"
#endif
#include <direct.h>
#define getcwd _getcwd
#else
#include <unistd.h>
#endif

using namespace std;

namespace htmake
{
	const string& Escape(const string& str);
}

const string& htmake::Escape(const string& str)
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

int main(int argc, const char * argv[])
{
	char pathBuffer[256];
	getcwd(pathBuffer, sizeof(pathBuffer));
	/*
		parse arguments
	*/

    fl_message("This is a test");
    /*
	fs::path baseDir = fs::u8path(argv[0]).parent_path();
	for (const fs::path& path : fs::directory_iterator(baseDir))
	{
		cout << "<p>" << path << "</p>" << endl;
	}
    string str(UTF8("(Lübeck & Schwerin) < Köln > Düsseldorf"));
    cout << "<p>" << makeHtmlSafe(str) << "</p>" << endl;
    */
    return 0;
}
