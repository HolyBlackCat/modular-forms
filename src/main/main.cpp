#include <iostream>

#include "interface/window.h"
#include "graphics/complete.h"
#include "program/exit.h"
#include "utils/clock.h"
#include "utils/dynamic_storage.h"
#include "utils/mat.h"
#include "utils/metronome.h"

#include <imgui.h>
#include <imgui_freetype.h>
#include <imgui_impl_opengl2.h>
#include <imgui_impl_sdl.h>

Interface::Window window;

namespace VisualOptions
{
    constexpr int
        outer_margin = 16;

    constexpr float
        step_list_width_relative = 0.2,
        step_list_min_width_pixels = 30,
        step_list_max_width_relative = 0.5;

    void GuiStyle(ImGuiStyle &style)
    {
        ImGui::StyleColorsLight();

        style.WindowPadding     = fvec2(10,10);
        style.FramePadding      = fvec2(12,3);
        style.ItemSpacing       = fvec2(16,6);
        style.ItemInnerSpacing  = fvec2(6,4);
        style.TouchExtraPadding = fvec2(0,0);
        style.IndentSpacing     = 26;
        style.ScrollbarSize     = 20;
        style.GrabMinSize       = 16;

        style.WindowBorderSize  = 1;
        style.ChildBorderSize   = 1;
        style.PopupBorderSize   = 1;
        style.FrameBorderSize   = 1;
        style.TabBorderSize     = 1;

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

namespace Data
{
    ReflectStruct(Procedure ,(
        (std::string)(name),
    ))
}

struct State
{
    virtual void Tick() = 0;
    virtual ~State() = default;
};

struct StateMain : State
{
    bool first_tick = 1;

    void Tick() override
    {
        Data::Procedure proc;
        proc.name = "Проверка системы";

        constexpr int window_flags =
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_HorizontalScrollbar;

        ImGui::SetNextWindowPos(ivec2(VisualOptions::outer_margin));
        ImGui::SetNextWindowSize(window.Size() - ivec2(VisualOptions::outer_margin * 2));

        bool keep_running = 1;
        ImGui::Begin((proc.name + "###procedure").c_str(), &keep_running, window_flags);
        if (!keep_running)
            Program::Exit();

        ImGui::Columns(2, "main_columns", 1);

        if (first_tick)
            ImGui::SetColumnWidth(-1, iround(window.Size().x * VisualOptions::step_list_width_relative));

        int list_column_width = ImGui::GetColumnWidth(-1);
        if (list_column_width < VisualOptions::step_list_min_width_pixels)
            ImGui::SetColumnWidth(-1, VisualOptions::step_list_min_width_pixels);
        else if (int max_list_column_width = iround(VisualOptions::step_list_max_width_relative * window.Size().x); list_column_width > max_list_column_width)
            ImGui::SetColumnWidth(-1, max_list_column_width);

        ImGui::BeginChild("step list");
        for (int i = 0; i < 100; i++)
            ImGui::Selectable(Str("Пункт ", i).c_str());
        ImGui::EndChild();

        ImGui::NextColumn();

        ImGui::BeginChild("actions");
        ImGui::Button("Кнопка 1");
        ImGui::Button("Кнопка 2");
        ImGui::EndChild();

        ImGui::End();

        //ImGui::ShowDemoWindow();

        first_tick = 0;
    }
};

int main()
{
    { // Initialize
        { // Window
            ivec2 window_size(800, 600);

            Interface::WindowSettings window_settings;
            window_settings.min_size = window_size / 2;
            window_settings.gl_major = 2;
            window_settings.gl_minor = 1;
            window_settings.gl_profile = Interface::Profile::any;

            window = Interface::Window("Modular forms", window_size, Interface::windowed, window_settings);
        }

        { // GUI
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

            ImFontGlyphRangesBuilder glyph_ranges_builder;
            glyph_ranges_builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
            glyph_ranges_builder.AddRanges(io.Fonts->GetGlyphRangesCyrillic());
            ImVector<ImWchar> glyph_ranges;
            glyph_ranges_builder.BuildRanges(&glyph_ranges);

            if (!io.Fonts->AddFontFromFileTTF("assets/Roboto-Regular.ttf", 20.0f, 0, glyph_ranges.begin()))
                Program::Error("Unable to load font.");

            ImGuiFreeType::BuildFontAtlas(io.Fonts, ImGuiFreeType::MonoHinting);

            VisualOptions::GuiStyle(ImGui::GetStyle());

            ImGui_ImplSDL2_InitForOpenGL(window.Handle(), window.Context());
            ImGui_ImplOpenGL2_Init();
        }
    }

    Metronome metronome(60);
    Clock::DeltaTimer delta_timer;

    Graphics::SetClearColor(fvec3(1));

    Dyn<State> state = state.make<StateMain>();

    while (1)
    {
        uint64_t delta = delta_timer();

        bool imgui_frame_started = 0;
        while (metronome.Tick(delta))
        {
            window.ProcessEvents([](const SDL_Event &event)
            {
                ImGui_ImplSDL2_ProcessEvent(&event);
            });

            if (window.Resized())
                Graphics::Viewport(window.Size());
            if (window.ExitRequested())
                Program::Exit();

            if (imgui_frame_started)
                ImGui::EndFrame();

            ImGui_ImplOpenGL2_NewFrame();
            ImGui_ImplSDL2_NewFrame(window.Handle());
            ImGui::NewFrame();
            imgui_frame_started = 1;

            state->Tick();

        }

        if (imgui_frame_started)
            ImGui::Render();

        Graphics::Clear();

        if (window.Ticks() > 1)
            ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

        window.SwapBuffers();
    }
}
