#include "file_selector.h"

#include "main/data.h"
#include "main/common.h"
#include "main/visual_options.h"

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
                state.directory_contents.push_back({elem.path().lexically_relative(state.current_path), elem.is_directory()});
            }

            std::sort(state.directory_contents.begin(), state.directory_contents.end(), [](const DirElement &a, const DirElement &b)
            {
                if (int x = a.is_directory - b.is_directory)
                    return x > 0;
                return a.name < b.name;
            });
        }
    }
    catch (std::exception &e)
    {
        state = old_state;
    }
}

void GuiElements::FileSelector::Open()
{
    should_open = 1;
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
        ImGui::SetNextWindowPos(ivec2(VisualOptions::image_preview_outer_margin));
        ImGui::SetNextWindowSize(ivec2(window.Size() - 2 * VisualOptions::image_preview_outer_margin));

        if (ImGui::BeginPopupModal(modal_name, 0, VisualOptions::modal_window_flags))
        {
            if (!modal_open)
            {
                modal_open = 1;
                // Initialize stuff.
            }

            if (state.current_path.empty())
                ImGui::TextDisabled("%s", "Диски");
            else
                ImGui::TextUnformatted(state.current_path.string().c_str());

            ImGui::BeginChildFrame(ImGui::GetID("hello"), ImGui::GetContentRegionAvail());

            if (state.directory_contents.empty())
            {
                if (state.is_invalid)
                    ImGui::TextDisabled("%s", "Невозможно открыть.");
            }
            else
            {
                bool in_root_dir = state.current_path.relative_path().empty();

                bool show_double_dot = !state.current_path.empty();

                NotOnPlatform(WINDOWS)
                (
                    if (in_root_dir)
                        show_double_dot = 0;
                )

                if (show_double_dot)
                {
                    if (ImGui::Selectable("..###back"))
                    {
                        if (in_root_dir)
                            SetNewPath(fs::path{});
                        else
                            SetNewPath(state.current_path / "..");
                    }
                    ImGui::Separator();
                }

                bool last_entry_was_dir = 0;

                int elem_index = 0;
                for (const auto &it : state.directory_contents)
                {
                    NotOnPlatform(WINDOWS)
                    (
                        if (it.is_dotdot)
                            continue;
                    )

                    if (!it.is_directory && last_entry_was_dir)
                        ImGui::Separator();
                    last_entry_was_dir = it.is_directory;

                    if (ImGui::Selectable(Str(Data::EscapeStringForWidgetName(it.name), "###", elem_index++).c_str()) && it.is_directory)
                    {
                        SetNewPath(state.current_path / it.path);
                    }
                }
            }

            ImGui::EndChildFrame();


            ImGui::EndPopup();
        }
    }
    else
    {
        modal_open = 0;
    }
}
