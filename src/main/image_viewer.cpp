#include "image_viewer.h"

#include <imgui.h>

#include "main/common.h"
#include "main/data.h"
#include "main/visual_options.h"

void GuiElements::ImageViewer::Display()
{
    if (ImGui::IsPopupOpen(modal_name))
    {
        ImGui::SetNextWindowPos(ivec2(VisualOptions::image_preview_outer_margin));
        ImGui::SetNextWindowSize(ivec2(window.Size() - 2 * VisualOptions::image_preview_outer_margin));

        if (ImGui::BeginPopupModal(modal_name, 0, VisualOptions::modal_window_flags))
        {
            const std::string text_close = "Закрыть";

            const auto &image = *Data::last_clicked_image;
            ivec2 available_size = iround(fvec2(ImGui::GetContentRegionAvail())).sub_y(ImGui::GetFrameHeightWithSpacing() + 2);

            fvec2 relative_image_size = image.pixel_size / fvec2(available_size);
            constexpr float max_scale = 8;
            float default_scale = clamp_max(1 / relative_image_size.max(), max_scale);
            float min_scale = min(default_scale * 0.75, 1);
            float min_scale_power = std::log2(min_scale), max_scale_power = std::log2(max_scale);

            { // Initialize viewer
                if (!modal_open)
                {
                    modal_open = 1;
                    scale_power = 0;
                    offset = fvec2(0);
                    dragging_now = 0;
                    scale_power = std::log2(default_scale);
                }
            }

            scale = pow(2, scale_power);

            ImGui::PushItemWidth(round(ImGui::GetWindowContentRegionWidth() / 3));
            ImGui::SliderFloat("Масштаб", &scale_power, min_scale_power, max_scale_power, Str(iround(scale * 100), "%%").c_str());
            ImGui::PopItemWidth();
            clamp_var(scale_power, min_scale_power, max_scale_power);

            ImGui::SameLine();
            int close_button_width = ImGui::CalcTextSize(text_close.c_str()).x + ImGui::GetStyle().FramePadding.x * 2;
            ImGui::SetCursorPosX(ImGui::GetWindowContentRegionWidth() - close_button_width);
            if (ImGui::Button(text_close.c_str()) || Input::Button(Input::escape).pressed())
            {
                ImGui::CloseCurrentPopup();
            }

            { // Drag image
                // This code has to be located here because of `GetCursorPos()`.
                fvec2 mouse_relative_pos = (mouse.pos() - fvec2(ImGui::GetCursorScreenPos())) / available_size;
                if (mouse.left.pressed() && (mouse_relative_pos >= 0).all() && (mouse_relative_pos <= 1).all())
                {
                    dragging_now = 1;
                    drag_click_pos = mouse_relative_pos;
                    drag_initial_offset = offset;
                }
                if (mouse.left.up() || !window.HasMouseFocus())
                {
                    dragging_now = 0;
                }
                if (dragging_now)
                {
                    offset = drag_initial_offset + (mouse_relative_pos - drag_click_pos) / scale * available_size;
                    clamp_var(offset, -image.pixel_size/2, image.pixel_size/2);
                }
            }

            ivec2 image_pixel_offset = iround(offset * scale);

            ivec2 image_visual_size = image.pixel_size * scale;

            ivec2 window_coord_a = image_pixel_offset + (available_size - image_visual_size) / 2;
            ivec2 window_coord_b = window_coord_a + image_visual_size;
            fmat3 m = linear_mapping<fvec2>(window_coord_a, window_coord_b, fvec2(0), fvec2(1)).matrix();

            fvec2 tex_coord_a = (m * ivec2(0).to_vec3(1)).to_vec2();
            fvec2 tex_coord_b = (m * available_size.to_vec3(1)).to_vec2();

            ImGui::Image(image.TextureHandle(), available_size, tex_coord_a, tex_coord_b, fvec4(1), ImGui::GetStyleColorVec4(ImGuiCol_Border));

            ImGui::EndPopup();
        }
    }
    else
    {
        modal_open = 0;
    }
}