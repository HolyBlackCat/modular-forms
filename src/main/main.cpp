#include <iostream>

#include <cstring>
#include <iomanip>
#include <memory>
#include <set>

#include "graphics/complete.h"
#include "input/complete.h"
#include "interface/window.h"
#include "program/exit.h"
#include "reflection/complete.h"
#include "utils/adjust.h"
#include "utils/clock.h"
#include "utils/dynamic_storage.h"
#include "utils/json.h"
#include "utils/mat.h"
#include "utils/meta.h"
#include "utils/metronome.h"
#include "utils/unicode.h"

#include "main/common.h"
#include "main/data.h"
#include "main/image_viewer.h"
#include "main/visual_options.h"

#include <imgui.h>
#include <imgui_freetype.h>
#include <imgui_impl_opengl2.h>
#include <imgui_impl_sdl.h>

Interface::Window window;
Input::Mouse mouse;

struct State
{
    bool exit_requested = 0;

    virtual void Tick() = 0;
    virtual ~State() = default;
};

struct StateMain : State
{
    ReflectStruct(ProcedureStep,(
        (std::string)(name),
        (std::optional<bool>)(confirm),
        (std::vector<Data::Widget>)(widgets),
    ))

    ReflectStruct(Procedure,(
        (std::string)(name),
        (std::optional<bool>)(confirm_exit),
        (std::vector<ProcedureStep>)(steps),
    ))

    bool first_tick = 1;
    std::string current_file_name;
    Procedure proc;
    std::size_t current_step_index = 0;
    bool first_tick_at_this_step = 1;

    GuiElements::ImageViewer image_viewer;

    StateMain()
    {
        Load("test_procedure.json");
    }

    void Load(std::string file_name)
    {
        try
        {
            Json json(MemoryFile(file_name).string(), 64);
            proc = Data::ReflectedObjectFromJson<Procedure>(json);

            if (proc.name.find_first_of("\n") != std::string::npos)
                Program::Error("Line feeds are not allowed in procedure names.");
            if (proc.steps.size() < 1)
                Program::Error("The procedure must have at least one step.");

            for (const ProcedureStep &step : proc.steps)
            {
                if (step.name.find_first_of("\n") != std::string::npos)
                    Program::Error("Line feeds are not allowed in procedure names.");
            }
        }
        catch (std::exception &e)
        {
            Program::Error("While loading `", file_name, "`: ", e.what());
        }

        current_file_name = file_name;
    }

    void Tick() override
    {
        ProcedureStep &current_step = proc.steps[current_step_index];

        constexpr int window_flags =
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_HorizontalScrollbar |
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_MenuBar;

        ImGui::SetNextWindowPos(fvec2(0));
        ImGui::SetNextWindowSize(window.Size());

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);

        ImGui::Begin("###procedure", 0, window_flags);

        ImGui::PopStyleVar(2);

        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("Файл"))
            {
                if (ImGui::MenuItem("Открыть"))
                    std::cout << "[Open file]\n";

                if (ImGui::MenuItem("Выйти"))
                    exit_requested = 1;

                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }

        ImGui::Columns(2, "main_columns", 1);

        if (first_tick)
        {
            int list_column_width = 0;

            for (const ProcedureStep &step : proc.steps)
                clamp_var_min(list_column_width, ImGui::CalcTextSize(step.name.c_str()).x);

            list_column_width += ImGui::GetStyle().ScrollbarSize + ImGui::GetStyle().ItemSpacing.x * 2;

            ImGui::SetColumnWidth(-1, list_column_width);
        }

        int list_column_width = ImGui::GetColumnWidth(-1);
        if (list_column_width < VisualOptions::step_list_min_width_pixels)
            ImGui::SetColumnWidth(-1, VisualOptions::step_list_min_width_pixels);
        else if (int max_list_column_width = iround(VisualOptions::step_list_max_width_relative * ImGui::GetWindowContentRegionWidth()); list_column_width > max_list_column_width)
            ImGui::SetColumnWidth(-1, max_list_column_width);

        { // Step list
            ImGui::BeginChild("step_list");

            std::size_t cur_step = current_step_index - (current_step_index > 0 ? first_tick_at_this_step : 0); // We use this rather than `current_step_index` to prevent jitter of the list when changing steps.

            for (std::size_t i = 0; i < proc.steps.size(); i++)
            {
                ImGui::Selectable(Data::EscapeStringForWidgetName(proc.steps[i].name).c_str(), i == cur_step, i > cur_step ? ImGuiSelectableFlags_Disabled : 0);
                if (first_tick_at_this_step && i == cur_step)
                    ImGui::SetScrollHereY(0.75);
            }

            ImGui::EndChild();
        }

        ImGui::NextColumn();

        ImGui::TextUnformatted(current_step.name.c_str());

        int bottom_panel_h = ImGui::GetFrameHeightWithSpacing(); // ImGui console example suggests this.
        ImGui::BeginChild(Str("widgets:", current_step_index).c_str(), fvec2(0, -bottom_panel_h), 1);

        int widget_index = 0;
        for (Data::Widget &widget : current_step.widgets)
        {
            if (widget_index != 0)
                ImGui::Spacing();
            widget->Display(widget_index++);

        }

        // Don't move this code below `EndChild()`, otherwise the modals won't open.
        image_viewer.Display();

        ImGui::EndChild();

        bool finishing_step_at_this_tick = 0;

        { // Ending step button and modal
            auto EndStep = [&]
            {
                current_step_index++;
                finishing_step_at_this_tick = 1;

                if (current_step_index >= proc.steps.size())
                    Program::Exit();
            };

            if (ImGui::Button("Завершить шаг"))
            {
                if (current_step.confirm && *current_step.confirm)
                    ImGui::OpenPopup("end_step_modal");
                else
                    EndStep();
            }

            if (ImGui::BeginPopupModal("end_step_modal", 0, VisualOptions::modal_window_flags))
            {
                ImGui::TextUnformatted("Действительно завершить шаг?");
                if (ImGui::Button("Завершить"))
                {
                    EndStep();
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Отмена"))
                {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }

        { // Exit modal
            auto Exit = [&]
            {
                Data::ReflectedObjectToJsonFile(proc, current_file_name);
                Program::Exit();
            };

            if (exit_requested)
            {
                exit_requested = 0;

                if (proc.confirm_exit && *proc.confirm_exit)
                    ImGui::OpenPopup("confirm_exit_modal");
                else
                    Exit();
            }

            if (ImGui::BeginPopupModal("confirm_exit_modal", 0, VisualOptions::modal_window_flags))
            {
                ImGui::TextUnformatted("Прервать действие и выйти?");
                if (ImGui::Button("Выйти"))
                {
                    Exit();
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Отмена"))
                {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }

        ImGui::End();

        first_tick = 0;
        first_tick_at_this_step = finishing_step_at_this_tick;
        finishing_step_at_this_tick = 0;

        // ImGui::ShowDemoWindow();
    }
};

int main()
{
    { // Initialize
        { // Window
            ivec2 window_size(800, 600);

            Interface::WindowSettings window_settings;
            window_settings.min_size = window_size;
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
            glyph_ranges_builder.AddText(Data::zero_width_space.c_str());
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
        bool imgui_frame_started = 0;
        while (metronome.Tick(delta_timer()))
        {
            window.ProcessEvents([](const SDL_Event &event)
            {
                ImGui_ImplSDL2_ProcessEvent(&event);
            });

            if (window.Resized())
                Graphics::Viewport(window.Size());
            if (window.ExitRequested())
                state->exit_requested = 1;

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

        uint64_t delta = Clock::Time() - delta_timer.LastTimePoint();
        double delta_secs = Clock::TicksToSeconds(delta);
        uint64_t target_delta_secs = 1.0 / metronome.Frequency();
        if (target_delta_secs > delta_secs)
            Clock::WaitSeconds((target_delta_secs - delta_secs) * 0.75);

        window.SwapBuffers();
    }
}
