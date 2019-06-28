#pragma once

#include <string>
#include <optional>
#include <vector>
#include <utility>

#include "options.h"

namespace FileDialogs
{
    std::optional<std::string> Open(std::string title, const std::vector<std::pair<std::string, std::string>> &filters, std::string default_path = "");
    std::optional<std::string> Save(std::string title, const std::vector<std::pair<std::string, std::string>> &filters, std::string default_path = "");

    inline std::optional<std::string> OpenTemplate()
    {
        return FileDialogs::Open("Открытие шаблона", {{"Файлы шаблонов", Options::template_extension}, {"Все файлы", ".*"}}, Options::template_dir);
    }

    inline std::optional<std::string> SaveTemplate()
    {
        return FileDialogs::Save("Сохранение шаблона", {{"Файлы шаблонов", Options::template_extension}, {"Все файлы", ".*"}}, Options::template_dir);
    }

    inline std::optional<std::string> OpenReport()
    {
        return FileDialogs::Open("Открытие отчета", {{"Файлы отчетов", Options::report_extension}, {"Все файлы", ".*"}});
    }

    inline std::optional<std::string> SaveReport()
    {
        return FileDialogs::Save("Сохранение отчета", {{"Файлы отчетов", Options::report_extension}, {"Все файлы", ".*"}});
    }
}
