#pragma once

#include <filesystem>
#include <string>
#include <vector>
namespace fs = std::filesystem;

namespace GuiElements
{
    class FileSelector
    {
        bool should_open = 0;
        bool modal_open = 0;

        struct DirElement
        {
            fs::path path;
            std::string name;
            bool is_directory = 0;

            DirElement() {}
            DirElement(fs::path path, bool is_directory) : path(path), name(path.string()), is_directory(is_directory) {}
        };

        struct State
        {
            fs::path current_path;

            std::vector<DirElement> directory_contents;
            bool is_invalid = 0;
        };
        State state;

        void SetNewPath(fs::path new_path);

      public:
        FileSelector();

        void Open();
        void Display();

        inline static constexpr const char *modal_name = "file_selector_modal";
    };
}
