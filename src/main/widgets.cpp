#include <string>
#include <iostream>

#include <imgui.h>

#include "interface/messagebox.h"

#include "main/images.h"
#include "main/gui_strings.h"
#include "main/options.h"
#include "main/procedure_data.h"

//{ Pull some internal ImGui functions.
// ImGui doesn't expose 'disable interactions' feature in the public headers, so we copy some declarations from an internal header.

typedef int ImGuiItemFlags;

enum ImGuiItemFlags_
{
    ImGuiItemFlags_NoTabStop                = 1 << 0,  // false
    ImGuiItemFlags_ButtonRepeat             = 1 << 1,  // false    // Button() will return true multiple times based on io.KeyRepeatDelay and io.KeyRepeatRate settings.
    ImGuiItemFlags_Disabled                 = 1 << 2,  // false    // [BETA] Disable interactions but doesn't affect visuals yet. See github.com/ocornut/imgui/issues/211
    ImGuiItemFlags_NoNav                    = 1 << 3,  // false
    ImGuiItemFlags_NoNavDefaultFocus        = 1 << 4,  // false
    ImGuiItemFlags_SelectableDontClosePopup = 1 << 5,  // false    // MenuItem/Selectable() automatically closes current Popup window
    ImGuiItemFlags_Default_                 = 0
};

namespace ImGui
{
    IMGUI_API void PushItemFlag(ImGuiItemFlags option, bool enabled);
    IMGUI_API void PopItemFlag();
}
//}

namespace Widgets
{
    class InteractionGuard
    {
      public:
        enum Mode {normal, visuals_only};

        InteractionGuard(bool widget_active, Mode mode = normal)
        {
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, !widget_active && mode != visuals_only);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, widget_active ? 1 : 0.5);
        }

        ~InteractionGuard()
        {
            ImGui::PopStyleVar();
            ImGui::PopItemFlag();
        }

        InteractionGuard(const InteractionGuard &) = delete;
        InteractionGuard &operator=(const InteractionGuard &) = delete;
    };

    STRUCT( Text EXTENDS Widgets::BasicWidget )
    {
        MEMBERS(
            DECL(std::string) text
        )

        void Display(int index, bool allow_modification) override
        {
            (void)index;
            (void)allow_modification;
            ImGui::PushTextWrapPos(); // Enable word wrapping.
            ImGui::TextUnformatted(text.c_str());
            ImGui::PopTextWrapPos();
        }
    };

    STRUCT( Spacing EXTENDS Widgets::BasicWidget )
    {
        MEMBERS()

        void Display(int index, bool allow_modification) override
        {
            (void)index;
            (void)allow_modification;
            for (int i = 0; i < 4; i++)
                ImGui::Spacing();
        }
    };

    STRUCT( Line EXTENDS Widgets::BasicWidget )
    {
        MEMBERS()

        void Display(int index, bool allow_modification) override
        {
            (void)index;
            (void)allow_modification;
            ImGui::Separator();
        }
    };

    STRUCT( ButtonList EXTENDS Widgets::BasicWidget )
    {
        struct Function
        {
            MEMBERS(
                DECL(std::string) library_id, func_id
            )

            Data::external_func_ptr_t ptr;

            Function() : ptr(nullptr) {}
        };

        SIMPLE_STRUCT( Button
            DECL(std::string) label
            DECL(std::string ATTR Refl::Optional) tooltip
            DECL(std::optional<Function> ATTR Refl::Optional) function
        )

        MEMBERS(
            DECL(std::vector<Button>) buttons
            DECL(bool INIT=false ATTR Refl::Optional) packed
        )

        int size_x = 0; // If `packed == true`, this is set to -1 in `Init()`, and then to the button width on the first `Display()` call. Otherwise it stays at 0.

        void Init(const Data::Procedure &proc) override
        {
            if (buttons.size() == 0)
                Program::Error("A button list must contain at least one button.");

            for (Button &button : buttons)
            {
                if (!button.function)
                    continue;

                if (!proc.libraries)
                    Program::Error("At least one button needs a function from a shared library, but no shared library list is provided.");

                auto lib_it = std::find_if(proc.libraries->begin(), proc.libraries->end(), [&](const Data::Library &lib){return lib.id == button.function->library_id;});
                if (lib_it == proc.libraries->end())
                    Program::Error("Shared library with id `", button.function->library_id, "` not found in the list of shared libraries.");

                auto func_it = std::find_if(lib_it->functions.begin(), lib_it->functions.end(), [&](const Data::LibraryFunc &func){return func.id == button.function->func_id;});
                if (func_it == lib_it->functions.end())
                    Program::Error("Function with id `", button.function->func_id, "` not found in shared library `", button.function->library_id, "`.");

                button.function->ptr = func_it->ptr;
            }

            // We can't calculate proper width here, as fonts don't seem to be loaded this early.
            if (packed)
                size_x = -1;
        }

        void Display(int index, bool allow_modification) override
        {
            if (size_x == -1)
            {
                for (const auto &button : buttons)
                    clamp_var_min(size_x, int(ImGui::CalcTextSize(button.label.c_str()).x));
                size_x += ImGui::GetStyle().FramePadding.x*2;
            }

            int size_with_padding = size_x + ImGui::GetStyle().ItemSpacing.x;

            int columns = size_x ? clamp_min(int(ImGui::GetWindowContentRegionWidth()) / size_with_padding, 1) : 1;
            int elems_per_column = columns != 1 ? (buttons.size() + columns - 1) / columns : buttons.size();
            columns = (buttons.size() + elems_per_column - 1) / elems_per_column;

            ImGui::Columns(columns, 0, 0);

            int cur_y = ImGui::GetCursorPosY();

            int elem_index = 0;
            for (int i = 0; i < columns; i++)
            {
                for (int j = 0; j < elems_per_column; j++)
                {
                    if (elem_index >= int(buttons.size()))
                        break; // I don't think we want to terminate the outer loop early. We want to use all columns no matter what.

                    const auto &button = buttons[elem_index];

                    bool has_function = bool(button.function && button.function->ptr);

                    { // Display button.
                        InteractionGuard interaction_guard(allow_modification && has_function);

                        if (ImGui::Button(Str(Data::EscapeStringForWidgetName(button.label), "###", index, ":", elem_index).c_str(), fvec2(size_x,0)) && allow_modification && has_function)
                        {
                            const char *error_message = button.function->ptr();
                            if (error_message)
                                Interface::MessageBox(Interface::MessageBoxType::warning, "Error", error_message);
                        }
                    }

                    if (button.tooltip.size() > 0 && ImGui::IsItemHovered())
                    {
                        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, fvec2(Options::Visual::tooltip_padding));
                        ImGui::BeginTooltip();
                        ImGui::TextUnformatted(button.tooltip.c_str());
                        ImGui::EndTooltip();
                        ImGui::PopStyleVar();
                    }

                    elem_index++;
                }

                ImGui::NextColumn();
                if (i != columns-1)
                    ImGui::SetCursorPosY(cur_y);
            }
        }
    };

    STRUCT( CheckBoxList EXTENDS Widgets::BasicWidget )
    {
        SIMPLE_STRUCT( CheckBox
            DECL(std::string) label
            DECL(bool INIT=false) state
            DECL(std::string ATTR Refl::Optional) tooltip
        )

        MEMBERS(
            DECL(std::vector<CheckBox>) checkboxes
            DECL(bool INIT=false ATTR Refl::Optional) packed
        )

        struct State
        {
            bool state = 0;
        };

        int size_x = 0; // If `packed == true`, this is set to -1 in `Init()`, and then to the column width on the first `Display()` call. Otherwise it stays at 0.

        void Init(const Data::Procedure &) override
        {
            if (checkboxes.size() == 0)
                Program::Error("A checkbox list must contain at least one checkbox.");

            // We can't calculate proper width here, as fonts don't seem to be loaded this early.
            if (packed)
                size_x = -1;
        }

        void Display(int index, bool allow_modification) override
        {
            if (size_x == -1)
            {
                for (const CheckBox &elem : checkboxes)
                    clamp_var_min(size_x, int(ImGui::CalcTextSize(elem.label.c_str()).x));

                size_x += ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.x + ImGui::GetStyle().ItemInnerSpacing.x;
            }

            int columns = size_x ? clamp_min(int(ImGui::GetWindowContentRegionWidth()) / size_x, 1) : 1;
            int elems_per_column = columns != 1 ? (checkboxes.size() + columns - 1) / columns : checkboxes.size();
            columns = (checkboxes.size() + elems_per_column - 1) / elems_per_column;

            ImGui::Columns(columns, 0, 0);

            int cur_y = ImGui::GetCursorPosY();

            InteractionGuard interaction_guard(allow_modification);

            int elem_index = 0;
            for (int i = 0; i < columns; i++)
            {
                for (int j = 0; j < elems_per_column; j++)
                {
                    if (elem_index >= int(checkboxes.size()))
                        break; // I don't think we want to terminate the outer loop early. We want to use all columns no matter what.

                    auto &checkbox = checkboxes[elem_index];

                    bool new_state = checkbox.state;
                    if (ImGui::Checkbox(Str(Data::EscapeStringForWidgetName(checkbox.label), "###", index, ":", elem_index).c_str(), &new_state) && allow_modification)
                    {
                        checkbox.state = new_state;
                    }

                    if (checkbox.tooltip.size() > 0 && ImGui::IsItemHovered())
                    {
                        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, fvec2(Options::Visual::tooltip_padding));
                        ImGui::BeginTooltip();
                        ImGui::TextUnformatted(checkbox.tooltip.c_str());
                        ImGui::EndTooltip();
                        ImGui::PopStyleVar();
                    }

                    elem_index++;
                }

                ImGui::NextColumn();
                if (i != columns-1)
                    ImGui::SetCursorPosY(cur_y);
            }
        }
    };

    STRUCT( RadioButtonList EXTENDS Widgets::BasicWidget )
    {
        SIMPLE_STRUCT( RadioButton
            DECL(std::string) label
            DECL(std::string ATTR Refl::Optional) tooltip
        )

        MEMBERS(
            DECL(std::vector<RadioButton>) radiobuttons
            DECL(int INIT=0) selected
            DECL(bool INIT=false ATTR Refl::Optional) packed
        )

        int size_x = 0; // If `packed == true`, this is set to -1 in `Init()`, and then to the column width on the first `Display()` call. Otherwise it stays at 0.

        void Init(const Data::Procedure &) override
        {
            if (radiobuttons.size() == 0)
                Program::Error("A checkbox list must contain at least one checkbox.");

            if (selected < 0 || selected > int(radiobuttons.size())) // Sic.
                Program::Error("Index of a selected radio button is out of range.");

            // We can't calculate a proper button width here, as fonts don't seem to be loaded this early.
            if (packed)
                size_x = -1;
        }

        void Display(int index, bool allow_modification) override
        {
            if (size_x == -1)
            {
                for (const auto &radiobutton : radiobuttons)
                    clamp_var_min(size_x, int(ImGui::CalcTextSize(radiobutton.label.c_str()).x));

                size_x += ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.x + ImGui::GetStyle().ItemInnerSpacing.x;
            }

            int columns = size_x ? clamp_min(int(ImGui::GetWindowContentRegionWidth()) / size_x, 1) : 1;
            int elems_per_column = columns != 1 ? (radiobuttons.size() + columns - 1) / columns : radiobuttons.size();
            columns = (radiobuttons.size() + elems_per_column - 1) / elems_per_column;

            ImGui::Columns(columns, 0, 0);

            int cur_y = ImGui::GetCursorPosY();

            InteractionGuard interaction_guard(allow_modification);

            int elem_index = 0;
            for (int i = 0; i < columns; i++)
            {
                for (int j = 0; j < elems_per_column; j++)
                {
                    if (elem_index >= int(radiobuttons.size()))
                        break; // I don't think we want to terminate the outer loop early. We want to use all columns no matter what.

                    const auto &radiobutton = radiobuttons[elem_index];

                    int new_selected = selected;
                    if (ImGui::RadioButton(Str(Data::EscapeStringForWidgetName(radiobutton.label), "###", index, ":", elem_index).c_str(), &new_selected, elem_index+1) && allow_modification)
                    {
                        if (selected != new_selected)
                            selected = new_selected;
                        else
                            selected = 0;
                    }

                    if (radiobutton.tooltip.size() > 0 && ImGui::IsItemHovered())
                    {
                        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, fvec2(Options::Visual::tooltip_padding));
                        ImGui::BeginTooltip();
                        ImGui::TextUnformatted(radiobutton.tooltip.c_str());
                        ImGui::EndTooltip();
                        ImGui::PopStyleVar();
                    }

                    elem_index++;
                }

                ImGui::NextColumn();
                if (i != columns-1)
                    ImGui::SetCursorPosY(cur_y);
            }
        }
    };

    STRUCT( TextInput EXTENDS Widgets::BasicWidget )
    {
        MEMBERS(
            DECL(std::string) label
            DECL(std::string) value
            DECL(std::string ATTR Refl::Optional) hint
            DECL(bool INIT=true ATTR Refl::Optional) inline_label
        )

        void Display(int index, bool allow_modification) override
        {
            InteractionGuard interaction_guard(allow_modification, InteractionGuard::visuals_only);

            if (!inline_label)
                ImGui::TextUnformatted(label.c_str());

            std::string inline_label_text = Str(inline_label ? Data::EscapeStringForWidgetName(label) : "", "###", index);

            if (hint.size() > 0)
                ImGui::InputTextWithHint(inline_label_text.c_str(), hint.c_str(), &value, !allow_modification * ImGuiInputTextFlags_ReadOnly);
            else
                ImGui::InputText(inline_label_text.c_str(), &value, !allow_modification * ImGuiInputTextFlags_ReadOnly);

        }
    };

    STRUCT( ImageList EXTENDS Widgets::BasicWidget )
    {
        struct Image
        {
            MEMBERS(
                DECL(std::string ATTR Refl::Optional) tooltip
                DECL(std::string) file_name
            )

            std::shared_ptr<Data::Image> data;

            ivec2 current_screen_size = ivec2(0);
        };

        MEMBERS(
            DECL(std::vector<Image>) images
            DECL(int INIT=1) columns
        )

        void Init(const Data::Procedure &proc) override
        {
            if (images.size() == 0)
                Program::Error("An image list must contain at least one image.");

            for (auto &image : images)
                image.data = Data::Image::Load((proc.resource_dir / image.file_name).string());
        }

        void Display(int index, bool allow_modification) override
        {
            (void)allow_modification;

            constexpr int padding = 2;

            int width = ImGui::GetWindowContentRegionWidth() / columns - ImGui::GetStyle().ItemSpacing.x - padding * 2;
            ivec2 max_size = ivec2(0);

            for (auto &image : images)
            {
                image.current_screen_size = iround(image.data->pixel_size / float(image.data->pixel_size.x) * width);
                clamp_var_min(max_size, image.current_screen_size);
            }

            int elems_per_column = columns != 1 ? (images.size() + columns - 1) / columns : images.size();

            ImGui::Columns(columns, 0, 0);

            int cur_y = ImGui::GetCursorPosY();

            int elem_index = 0;
            for (int i = 0; i < columns; i++)
            {
                for (int j = 0; j < elems_per_column; j++)
                {
                    if (elem_index >= int(images.size()))
                        break; // I don't think we want to terminate the outer loop early. We want to use all columns no matter what.

                    const auto &image = images[elem_index];

                    if (!image.data->texture)
                        Program::Error("Internal error: Image handle is null.");

                    fvec2 image_size_relative_to_button = image.current_screen_size / fvec2(max_size);
                    fvec2 coord_a = (1 / image_size_relative_to_button - 1) / -2;
                    fvec2 coord_b = 1 - coord_a;

                    ImGui::PushID(index); // We push this here rather than outside of the loop because we don't want it to affect modal window if we're going to open it.
                    ImGui::PushID(elem_index);
                    bool button_pressed = ImGui::ImageButton(image.data->TextureHandle(), max_size, coord_a, coord_b, padding);
                    ImGui::PopID();
                    ImGui::PopID();

                    if (image.tooltip.size() > 0 && ImGui::IsItemHovered())
                    {
                        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, fvec2(Options::Visual::tooltip_padding));
                        ImGui::BeginTooltip();
                        ImGui::TextUnformatted(image.tooltip.c_str());
                        ImGui::EndTooltip();
                        ImGui::PopStyleVar();
                    }

                    if (button_pressed)
                    {
                        Data::clicked_image = &*image.data;
                    }

                    elem_index++;
                }

                ImGui::NextColumn();
                if (i != columns-1)
                    ImGui::SetCursorPosY(cur_y);
            }
        }
    };
}
