#include "file_selector.h"

#include <imgui.h>

#include "utils/mat.h"

#include "main/common.h"
#include "main/visual_options.h"


void GuiElements::FileSelector::Display()
{


    if (ImGui::IsPopupOpen(modal_name))
    {
        ImGui::SetNextWindowPos(ivec2(VisualOptions::image_preview_outer_margin));
        ImGui::SetNextWindowSize(ivec2(window.Size() - 2 * VisualOptions::image_preview_outer_margin));

        if (ImGui::BeginPopupModal(modal_name, 0, VisualOptions::modal_window_flags))
        {
            if (!modal_open)
            {
                modal_open = 1;

                current_path = fs::current_path();
            }

            ImGui::EndPopup();
        }
    }
    else
    {
        modal_open = 0;
    }
}