#pragma once

#include <string>

namespace Data // Strings
{
    inline const std::string zero_width_space = "\xEF\xBB\xBF";

    // `##` has a special meaning for ImGui when used in widget names, so we break those with zero-width spaces.
    inline std::string EscapeStringForWidgetName(const std::string &source_str)
    {
        std::string ret;

        for (char ch : source_str)
        {
            ret += ch;
            if (ch == '#')
                ret += zero_width_space;
        }

        return ret;
    }
}
