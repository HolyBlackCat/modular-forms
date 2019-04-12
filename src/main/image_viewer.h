#pragma once

#include "utils/mat.h"

namespace GuiElements
{
    class ImageViewer
    {
        bool modal_open = 0;
        float scale_power = 0;
        fvec2 offset = fvec2(0);
        float scale = 1;
        bool dragging_now = 1;
        fvec2 drag_click_pos = fvec2(0);
        fvec2 drag_initial_offset = fvec2(0);

      public:
        void Display();

        inline static constexpr const char *modal_name = "image_view_modal";
    };
}