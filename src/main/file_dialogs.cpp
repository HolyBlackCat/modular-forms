#include "file_dialogs.h"

#include <utility>

#include <portable-file-dialogs.h>

#include "utils/format.h"

namespace FileDialogs
{
    std::optional<std::string> Open(std::string title, const std::vector<std::pair<std::string, std::string>> &filters, std::string default_path)
    {
        std::vector<std::string> filters_flat;
        for (const auto &[name, value] : filters)
        {
            filters_flat.push_back("{} (*{})"_format(name, value));
            filters_flat.push_back("*" + value);
        }

        pfd::open_file dialog(title, default_path, std::move(filters_flat));
        auto result = dialog.result();
        if (result.empty())
            return {};
        return result[0];
    }

    std::optional<std::string> Save(std::string title, const std::vector<std::pair<std::string, std::string>> &filters, std::string default_path)
    {
        std::vector<std::string> filters_flat;
        for (const auto &[name, value] : filters)
        {
            filters_flat.push_back("{} (*{})"_format(name, value));
            filters_flat.push_back("*" + value);
        }

        pfd::save_file dialog(title, default_path, std::move(filters_flat));
        auto result = dialog.result();
        if (result.empty())
            return {};
        return result;
    }
}
