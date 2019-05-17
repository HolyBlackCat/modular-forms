#pragma once

#include <filesystem>
#include <string>
#include <vector>
namespace fs = std::filesystem;

namespace GuiElements
{
    class FileSelector
    {
      public:
        enum class Mode {save_as, open};

      private:
        bool should_open = 0;
        bool modal_open = 0;
        Mode mode = Mode::open;
        std::vector<std::string> allowed_suffixes;

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
            int selected_entry = -1;

            unsigned int open_string_version = 0;
            std::string open_string; // This is owned by a textbox widget. If you modify this string, increment `open_string_version`.
        };
        State state;

        void SetNewPath(fs::path new_path);

        inline static constexpr const char *modal_name = "file_selector_modal";
      public:
        FileSelector();

        void Open(Mode new_mode, std::vector<std::string> new_allowed_suffixes = {});
        void Display();

        bool is_done = 0; // This is set to `true` when a file is selected and the modal is closed.
        fs::path result;
    };
}
