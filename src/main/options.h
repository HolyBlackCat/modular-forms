#pragma once

#include <imgui.h>

#include "utils/mat.h"

namespace Options
{
    inline const std::string template_extension = ".template", template_dir = "templates", report_extension = ".report";

    namespace Visual
    {
        inline constexpr float
            step_list_max_width_relative = 0.5;

        inline constexpr int
            modal_window_flags_with_title = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings,
            modal_window_flags = ImGuiWindowFlags_NoTitleBar | modal_window_flags_with_title,
            step_list_min_width_pixels = 96,
            image_preview_outer_margin = 48,
            tooltip_padding = 2;

        inline void GuiStyle(ImGuiStyle &style)
        {
            ImGui::StyleColorsLight();

            style.WindowPadding     = fvec2(8,8);
            style.FramePadding      = fvec2(8,2);
            style.ItemSpacing       = fvec2(10,6);
            style.ItemInnerSpacing  = fvec2(8,4);
            style.TouchExtraPadding = fvec2(0,0);
            style.IndentSpacing     = 26;
            style.ScrollbarSize     = 16;
            style.GrabMinSize       = 16;

            style.WindowBorderSize  = 1;
            style.ChildBorderSize   = 1;
            style.PopupBorderSize   = 1;
            style.FrameBorderSize   = 1;
            style.TabBorderSize     = 0;

            style.WindowRounding    = 5;
            style.ChildRounding     = 5;
            style.FrameRounding     = 5;
            style.PopupRounding     = 5;
            style.ScrollbarRounding = 3;
            style.GrabRounding      = 3;
            style.TabRounding       = 3;

            style.WindowTitleAlign  = fvec2(0.5,0.5);
        }
    }
}
