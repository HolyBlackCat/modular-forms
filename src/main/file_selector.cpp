#include "file_selector.h"

#include "main/data.h"
#include "main/common.h"
#include "main/options.h"

GuiElements::FileSelector::FileSelector()
{
    SetNewPath(fs::current_path());
}

void GuiElements::FileSelector::SetNewPath(fs::path new_path)
{
    State old_state = state;

    try
    {
        bool is_drive_list = 0;

        // Handle empty path.
        // On windows we treat it as a fake folder containing all drives.
        // On other platforms we replace it with the root directory.
        if (new_path.empty())
        {
            OnPlatform(WINDOWS)
            (
                is_drive_list = 1;
            )

            NotOnPlatform(WINDOWS)
            (
                new_path = "/";
            )
        }

        state.directory_contents = {};

        // If path was empty and we're on Windows, generate drive list and stop.
        if (is_drive_list)
        {
            state.current_path = fs::path{};

            for (char letter = 'A'; letter <= 'Z'; letter++)
            {
                std::string drive = Str(letter, ":\\");

                if (fs::exists(drive))
                    state.directory_contents.push_back({drive, 1});
            }

            return;
        }

        // Canonize path.
        state.current_path = fs::weakly_canonical(new_path);

        // Make sure the path is valid.
        state.is_invalid = !fs::exists(state.current_path);
        if (state.is_invalid)
            return;

        // Get directory elements.
        if (!state.is_invalid)
        {
            for (auto &elem : fs::directory_iterator(state.current_path, fs::directory_options::skip_permission_denied | fs::directory_options::follow_directory_symlink))
            {
                std::string path_string = elem.path().string();
                auto path_ends_with_suffix = [&](const std::string &suffix)
                {
                    return Strings::EndsWith(path_string, suffix);
                };

                bool allow = elem.is_directory() || allowed_suffixes.empty() || std::any_of(allowed_suffixes.begin(), allowed_suffixes.end(), path_ends_with_suffix);
                if (allow)
                    state.directory_contents.push_back({elem.path().lexically_relative(state.current_path), elem.is_directory()});
            }

            std::sort(state.directory_contents.begin(), state.directory_contents.end(), [](const DirElement &a, const DirElement &b)
            {
                if (int x = a.is_directory - b.is_directory)
                    return x > 0;
                return a.name < b.name;
            });
        }

        // Reset selected entry.
        state.selected_entry = -1;

        // Reset file name.
        state.open_string = {};
        state.open_string_version++;

        // Reset done flag.
        is_done = 0;
    }
    catch (std::exception &e)
    {
        state = old_state;
    }
}

void GuiElements::FileSelector::Open(Mode new_mode, std::vector<std::string> new_allowed_suffixes)
{
    mode = new_mode;
    allowed_suffixes = std::move(new_allowed_suffixes);
    should_open = 1;

    SetNewPath(state.current_path); // This updates directory contents with respect to the specified suffixes.
}

void GuiElements::FileSelector::Display()
{
    if (should_open)
    {
        ImGui::OpenPopup(modal_name);
        should_open = 0;
    }

    if (ImGui::IsPopupOpen(modal_name))
    {
        ImGui::SetNextWindowPos(ivec2(Options::Visual::image_preview_outer_margin));
        ImGui::SetNextWindowSize(ivec2(window.Size() - 2 * Options::Visual::image_preview_outer_margin));

        if (ImGui::BeginPopupModal(modal_name, 0, Options::Visual::modal_window_flags))
        {
            if (!modal_open)
            {
                modal_open = 1;
                // Initialize stuff.
            }

            ModeStrings mode_strings = GetModeStrings(mode);

            const std::string text_close = "Отмена";
            int close_button_width = ImGui::CalcTextSize(text_close.c_str()).x + ImGui::GetStyle().FramePadding.x * 2;
            int done_button_width = ImGui::CalcTextSize(mode_strings.button_confirm.c_str()).x + ImGui::GetStyle().FramePadding.x * 2;

            ImGui::TextUnformatted(mode_strings.window_title.c_str());

            ImGui::SameLine();

            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - close_button_width);
            if (ImGui::Button(text_close.c_str()) || Input::Button(Input::escape).pressed())
            {
                ImGui::CloseCurrentPopup();
            }

            ImGui::Separator();

            ivec2 path_area_size = ivec2(ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeightWithSpacing());

            ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 10);
            ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 0);
            ImGui::BeginChild("path_area", path_area_size, false, ImGuiWindowFlags_HorizontalScrollbar);

            if (state.current_path.empty())
                ImGui::TextDisabled("%s", "Диски");
            else
                ImGui::TextUnformatted(state.current_path.string().c_str());
            ImGui::EndChild();
            ImGui::PopStyleVar(2);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 6);

            ImGui::BeginChildFrame(ImGui::GetID(state.current_path.string().c_str()), ivec2(ImGui::GetContentRegionAvail()) with(y -= ImGui::GetFrameHeightWithSpacing()));

            bool in_root_dir = state.current_path.relative_path().empty();
            bool show_double_dot = !state.current_path.empty();

            NotOnPlatform(WINDOWS)
            (
                if (in_root_dir)
                    show_double_dot = 0;
            )
            
            fs::path new_path;

            if (show_double_dot)
            {
                if (ImGui::Selectable("..", state.selected_entry == int(state.directory_contents.size()), ImGuiSelectableFlags_AllowDoubleClick))
                {
                    state.selected_entry = state.directory_contents.size();

                    if (ImGui::IsMouseDoubleClicked(0))
                    {
                        if (in_root_dir)
                            new_path = fs::path{};
                        else
                            new_path = state.current_path / "..";
                    }
                }

                if (state.directory_contents.size() > 0 || state.is_invalid)
                    ImGui::Separator();
            }

            bool last_entry_was_dir = 0;

            int elem_index = 0;
            for (const auto &it : state.directory_contents)
            {
                if (!it.is_directory && last_entry_was_dir)
                    ImGui::Separator();
                last_entry_was_dir = it.is_directory;

                bool is_selected = elem_index == state.selected_entry;
                if (ImGui::Selectable(Str(Data::EscapeStringForWidgetName(it.name), "###", elem_index).c_str(), is_selected, ImGuiSelectableFlags_AllowDoubleClick))
                {
                    state.selected_entry = elem_index;

                    if (!it.is_directory)
                    {
                        state.open_string = it.name;
                        state.open_string_version++;
                    }

                    if (ImGui::IsMouseDoubleClicked(0))
                    {
                        if (it.is_directory)
                        {
                            new_path = state.current_path / it.path;
                        }
                        else
                        {
                            ImGui::CloseCurrentPopup();
                            is_done = 1;
                            result = fs::weakly_canonical(it.path);
                        }
                    }
                }

                elem_index++;
            }

            if (state.is_invalid)
                ImGui::TextDisabled("%s", "Невозможно открыть.");

            ImGui::EndChildFrame();

            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - done_button_width - ImGui::GetStyle().ItemSpacing.x);
            ImGui::InputTextWithHint(Str("###open_string:", state.open_string_version).c_str(), "Имя файла", &state.open_string);
            ImGui::SameLine();
            if (ImGui::Button(mode_strings.button_confirm.c_str()))
            {
                ImGui::CloseCurrentPopup();
                is_done = 1;
                result = fs::weakly_canonical(state.current_path / state.open_string);
            }

            ImGui::EndPopup();
            
            if (!new_path.empty())
            	SetNewPath(new_path);
        }
    }
    else
    {
        modal_open = 0;
    }
}
