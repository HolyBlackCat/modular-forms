#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace GuiElements
{
    class FileSelector
    {
        bool modal_open = 0;
        fs::path current_path;

      public:
        void Display();

        inline static constexpr const char *modal_name = "file_selector_modal";
    };
}