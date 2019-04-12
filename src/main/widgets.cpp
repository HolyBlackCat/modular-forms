#include "main/data.h"

#include <optional>
#include <set>
#include <string>

#include "graphics/texture.h"
#include "main/visual_options.h"
#include "program/errors.h"
#include "utils/mat.h"

#include <imgui.h>

namespace Widgets
{
    struct Text : Data::WidgetBase<Text>
    {
        inline static constexpr const char *internal_name = "text";

        Reflect(Text)
        (
            (std::string)(text),
        )

        void Display(int index) override
        {
            (void)index;
            ImGui::PushTextWrapPos(); // Enable word wrapping.
            ImGui::TextUnformatted(text.c_str());
            ImGui::PopTextWrapPos();
        }
    };

    struct Spacing : Data::WidgetBase<Spacing>
    {
        inline static constexpr const char *internal_name = "spacing";

        void Display(int index) override
        {
            (void)index;
            for (int i = 0; i < 4; i++)
                ImGui::Spacing();
        }
    };

    struct Line : Data::WidgetBase<Line>
    {
        inline static constexpr const char *internal_name = "line";

        void Display(int index) override
        {
            (void)index;
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
        }
    };

    struct ButtonList : Data::WidgetBase<ButtonList>
    {
        inline static constexpr const char *internal_name = "button_list";

        ReflectStruct(Button, (
            (std::string)(label),
            (std::optional<std::string>)(tooltip),
        ))

        Reflect(ButtonList)
        (
            (std::vector<Button>)(buttons),
            (std::optional<bool>)(packed),
        )

        int size_x = 0; // If `packed == true`, this is set to -1 in `Init()`, and then to the button width on the first `Display()` call. Otherwise it stays at 0.

        void Init() override
        {
            if (buttons.size() == 0)
                Program::Error("A button list must contain at least one button.");

            // We can't calculate proper width here, as fonts don't seem to be loaded this early.
            if (packed && *packed)
                size_x = -1;
        }

        void Display(int index) override
        {
            if (size_x == -1)
            {
                for (const auto &button : buttons)
                    clamp_var_min(size_x, ImGui::CalcTextSize(button.label.c_str()).x);
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

                    ImGui::Button(Str(Data::EscapeStringForWidgetName(button.label), "###", index, ":", elem_index).c_str(), fvec2(size_x,0));

                    if (button.tooltip && ImGui::IsItemHovered())
                    {
                        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, fvec2(VisualOptions::tooltip_padding));
                        ImGui::BeginTooltip();
                        ImGui::TextUnformatted(button.tooltip->c_str());
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

    struct CheckBoxList : Data::WidgetBase<CheckBoxList>
    {
        inline static constexpr const char *internal_name = "checkbox_list";

        ReflectStruct(CheckBox,(
            (std::string)(label),
            (bool)(state)(=0),
            (std::optional<std::string>)(tooltip),
        ))

        Reflect(CheckBoxList)
        (
            (std::vector<CheckBox>)(checkboxes),
            (std::optional<bool>)(packed),
        )

        struct State
        {
            bool state = 0;
        };

        int size_x = 0; // If `packed == true`, this is set to -1 in `Init()`, and then to the column width on the first `Display()` call. Otherwise it stays at 0.

        void Init() override
        {
            if (checkboxes.size() == 0)
                Program::Error("A checkbox list must contain at least one checkbox.");

            // We can't calculate proper width here, as fonts don't seem to be loaded this early.
            if (packed && *packed)
                size_x = -1;
        }

        void Display(int index) override
        {
            if (size_x == -1)
            {
                for (const CheckBox &elem : checkboxes)
                    clamp_var_min(size_x, ImGui::CalcTextSize(elem.label.c_str()).x);

                size_x += ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.x + ImGui::GetStyle().ItemInnerSpacing.x;
            }

            int columns = size_x ? clamp_min(int(ImGui::GetWindowContentRegionWidth()) / size_x, 1) : 1;
            int elems_per_column = columns != 1 ? (checkboxes.size() + columns - 1) / columns : checkboxes.size();
            columns = (checkboxes.size() + elems_per_column - 1) / elems_per_column;

            ImGui::Columns(columns, 0, 0);

            int cur_y = ImGui::GetCursorPosY();

            int elem_index = 0;
            for (int i = 0; i < columns; i++)
            {
                for (int j = 0; j < elems_per_column; j++)
                {
                    if (elem_index >= int(checkboxes.size()))
                        break; // I don't think we want to terminate the outer loop early. We want to use all columns no matter what.

                    auto &checkbox = checkboxes[elem_index];

                    ImGui::Checkbox(Str(Data::EscapeStringForWidgetName(checkbox.label), "###", index, ":", elem_index).c_str(), &checkbox.state);

                    if (checkbox.tooltip && ImGui::IsItemHovered())
                    {
                        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, fvec2(VisualOptions::tooltip_padding));
                        ImGui::BeginTooltip();
                        ImGui::TextUnformatted(checkbox.tooltip->c_str());
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

    struct RadioButtonList : Data::WidgetBase<RadioButtonList>
    {
        inline static constexpr const char *internal_name = "radiobutton_list";

        ReflectStruct(RadioButton, (
            (std::string)(label),
            (std::optional<std::string>)(tooltip),
        ))

        Reflect(RadioButtonList)
        (
            (std::vector<RadioButton>)(radiobuttons),
            (int)(selected)(=0),
            (std::optional<bool>)(packed),
        )

        int size_x = 0; // If `packed == true`, this is set to -1 in `Init()`, and then to the column width on the first `Display()` call. Otherwise it stays at 0.

        void Init() override
        {
            if (radiobuttons.size() == 0)
                Program::Error("A checkbox list must contain at least one checkbox.");

            if (selected < 0 || selected > int(radiobuttons.size())) // Sic.
                Program::Error("Index of a selected radio button is out of range.");

            // We can't calculate a proper button width here, as fonts don't seem to be loaded this early.
            if (packed && *packed)
                size_x = -1;
        }

        void Display(int index) override
        {
            if (size_x == -1)
            {
                for (const auto &radiobutton : radiobuttons)
                    clamp_var_min(size_x, ImGui::CalcTextSize(radiobutton.label.c_str()).x);

                size_x += ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.x + ImGui::GetStyle().ItemInnerSpacing.x;
            }

            int columns = size_x ? clamp_min(int(ImGui::GetWindowContentRegionWidth()) / size_x, 1) : 1;
            int elems_per_column = columns != 1 ? (radiobuttons.size() + columns - 1) / columns : radiobuttons.size();
            columns = (radiobuttons.size() + elems_per_column - 1) / elems_per_column;

            ImGui::Columns(columns, 0, 0);

            int cur_y = ImGui::GetCursorPosY();

            int elem_index = 0;
            for (int i = 0; i < columns; i++)
            {
                for (int j = 0; j < elems_per_column; j++)
                {
                    if (elem_index >= int(radiobuttons.size()))
                        break; // I don't think we want to terminate the outer loop early. We want to use all columns no matter what.

                    const auto &radiobutton = radiobuttons[elem_index];

                    ImGui::RadioButton(Str(Data::EscapeStringForWidgetName(radiobutton.label), "###", index, ":", elem_index).c_str(), &selected, elem_index+1);

                    if (radiobutton.tooltip && ImGui::IsItemHovered())
                    {
                        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, fvec2(VisualOptions::tooltip_padding));
                        ImGui::BeginTooltip();
                        ImGui::TextUnformatted(radiobutton.tooltip->c_str());
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

    struct TextInput : Data::WidgetBase<TextInput>
    {
        inline static constexpr const char *internal_name = "text_input";

        Reflect(TextInput)
        (
            (std::string)(label),
            (int)(max_bytes)(=2),
            (std::string)(value),
            (std::optional<std::string>)(hint),
            (std::optional<bool>)(inline_label),
        )

        // This is null-terminated. We use a vector because ImGui wants a fixed-size buffer.
        // It's probably possible to somehow directly plug `std::string` in there, but I don't want to bother.
        std::vector<char> value_vec;

        void Init() override
        {
            if (max_bytes < 2)
                Program::Error("Max amount of bytes for a text input box has to be at least 2.");

            if (int(value.size()) > max_bytes)
                Program::Error("Initial value for a text input box has more symbols than the selected max amount.");

            value_vec.resize(max_bytes);
            std::copy(value.begin(), value.end(), value_vec.begin());
        }

        void Display(int index) override
        {
            bool value_changed = 0;
            bool use_inline_label = inline_label ? *inline_label : 1;

            if (!use_inline_label)
                ImGui::TextUnformatted(label.c_str());

            std::string inline_label = Str(use_inline_label ? Data::EscapeStringForWidgetName(label) : "", "###", index);

            if (hint) // Note `tmp.size()+1` below. It's because `std::string` also includes space for a null-terminator.
                value_changed = ImGui::InputTextWithHint(inline_label.c_str(), hint->c_str(), value_vec.data(), value_vec.size());
            else
                value_changed = ImGui::InputText(inline_label.c_str(), value_vec.data(), value_vec.size());

            if (value_changed)
                value = value_vec.data();
        }
    };

    struct ImageList : Data::WidgetBase<ImageList>
    {
        inline static constexpr const char *internal_name = "image_list";

        struct Image
        {
            Reflect(Image)
            (
                (std::optional<std::string>)(tooltip),
                (std::string)(file_name),
            )

            std::shared_ptr<Data::Image> data;

            ivec2 current_screen_size = ivec2(0);
        };

        Reflect(ImageList)
        (
            (std::vector<Image>)(images),
            (int)(columns)(=1),
        )

        void Init() override
        {
            if (images.size() == 0)
                Program::Error("An image list must contain at least one image.");

            for (auto &image : images)
                image.data = Data::Image::Load(image.file_name);
        }

        void Display(int index) override
        {
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

                    ImGui::PushID(index);
                    bool button_pressed = ImGui::ImageButton(image.data->TextureHandle(), max_size, coord_a, coord_b, padding);
                    ImGui::PopID();

                    if (image.tooltip && ImGui::IsItemHovered())
                    {
                        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, fvec2(VisualOptions::tooltip_padding));
                        ImGui::BeginTooltip();
                        ImGui::TextUnformatted(image.tooltip->c_str());
                        ImGui::EndTooltip();
                        ImGui::PopStyleVar();
                    }

                    if (button_pressed)
                    {
                        ImGui::OpenPopup("image_view_modal");
                        Data::last_clicked_image = &*image.data;
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