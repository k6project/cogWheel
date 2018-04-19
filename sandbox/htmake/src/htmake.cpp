#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

const std::string&& make_html_safe(const std::string& str)
{
    using pattern_t = std::pair<std::string, std::string>;
    static const std::vector<pattern_t> HTML_ESCAPE_CHARS
    {
        {u8"&", u8"&amp;"}, {u8"ü", u8"&uuml;"}, {u8"Ü", u8"&Uuml;"},
        {u8">", u8"&gt;"}, {u8"ä", u8"&auml;"}, {u8"Ä", u8"&Auml;"},
        {u8"<", u8"&lt;"}, {u8"ö", u8"&ouml;"}, {u8"Ö", u8"&Ouml;"}
    };
    std::string result;
    result.reserve(str.size() << 1);
    for (auto it = str.begin(); it != str.end();)
    {
        size_t escaped = 0;
        const char* start = &*it;
        for (const auto& entry : HTML_ESCAPE_CHARS)
        {
            const std::string& pattern = entry.first;
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
    result.shrink_to_fit();
    return std::move(result);
}

int main(int argc, const char * argv[])
{
    std::string str("(Lübeck & Schwerin) < Köln > Düsseldorf");
    std::string result = make_html_safe(str);
    std::cout << "<p>" << result << "</p>" << std::endl;
    return 0;
}
