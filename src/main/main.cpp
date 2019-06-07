#include <iostream>

#include "main/common.h"
#include "main/file_selector.h"
#include "main/gui_strings.h"
#include "main/image_viewer.h"
#include "main/options.h"
#include "main/procedure_data.h"
#include "main/widgets.h"

namespace fs = std::filesystem;

Interface::Window window;
Interface::ImGuiController gui_controller;
Input::Mouse mouse;

struct State
{
    bool exit_requested = 0;

    virtual void Tick() = 0;
    virtual ~State() = default;
};

struct StateMain : State
{
    struct Tab
    {
        Data::Procedure proc;
        std::string pretty_name;
        fs::path path;
        bool first_tick = 1;

        int visible_step = 0;
        bool should_adjust_step_list_scrolling = 0;

        inline static unsigned int id_counter = 1;
        int id = id_counter++;

        void RegeneratePrettyName()
        {
            pretty_name = path.stem().string(); // `stem` means file name without extension.
        }
    };

    std::vector<Tab> tabs;
    int active_tab = -1;

    bool HaveActiveTab() const {return active_tab >= 0 && active_tab < int(tabs.size());}

    GuiElements::ImageViewer image_viewer;
    GuiElements::FileSelector file_selector;


    StateMain() {}

    void Load(fs::path path)
    {
        if (path.empty())
            return;

        try
        {
            fs::file_status status = fs::status(path);
            if (!fs::exists(status))
                Program::Error("File doesn't exist.");
            if (!fs::is_regular_file(status))
                Program::Error("Not a regular file.");
            if (path.extension() != Options::extension)
                Program::Error("Invalid extension, expected `", Options::extension, "`.");

            Tab new_tab;

            new_tab.path = path;
            new_tab.RegeneratePrettyName();

            Json json(MemoryFile(path.string()).string(), 64);
            new_tab.proc = Widgets::ReflectedObjectFromJson<Data::Procedure>(json.GetView());

            if (path.has_parent_path())
                new_tab.proc.base_dir = path.parent_path().generic_string() + '/';

            Widgets::InitializeWidgetsRecursively(new_tab.proc, new_tab.proc);

            if (new_tab.proc.steps.size() < 1)
                Program::Error("The procedure must have at least one step.");
            if (new_tab.proc.current_step < 0 || new_tab.proc.current_step >= int(new_tab.proc.steps.size()))
                Program::Error("Current step index is out of range.");

            new_tab.visible_step = new_tab.proc.current_step;

            tabs.push_back(std::move(new_tab));
        }
        catch (std::exception &e)
        {
            Interface::MessageBox(Interface::MessageBoxType::warning, "Error", Str("Can't load `", path.string(), "`:\n", e.what()));
        }
    }

    void Tick() override
    {
        for (const std::string &new_file : window.DroppedFiles())
            Load(new_file);

        if (file_selector.is_done)
        {
            file_selector.is_done = 0;
            switch (file_selector.CurrentMode())
            {
              case GuiElements::FileSelector::Mode::open:
                Load(file_selector.result);
                break;
              default:
                break;
            }
        }

        constexpr int window_flags =
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_HorizontalScrollbar |
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_MenuBar |
            ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::SetNextWindowPos(fvec2(0));
        ImGui::SetNextWindowSize(window.Size());

        ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 2);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ivec2(5));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ivec2(4,1));

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ivec2(0)); // Sic, we set it again.

        ImGui::Begin("###procedure", 0, window_flags);

        ImGui::PopStyleVar(3);


        { // Menu bar
            if (ImGui::BeginMenuBar())
            {
                if (ImGui::BeginMenu("Файл"))
                {
                    if (ImGui::MenuItem("Открыть"))
                        file_selector.Open(GuiElements::FileSelector::Mode::open, {".json"});

                    if (ImGui::MenuItem("Выйти"))
                        exit_requested = 1;

                    ImGui::EndMenu();
                }

                ImGui::EndMenuBar();
            }
        }

        ImGui::PopStyleVar(3);

        bool need_step_end_confirmation = 0;
        auto EndStep = [&]
        {
            if (!HaveActiveTab())
                return;

            Tab &tab = tabs[active_tab];

            tab.proc.current_step++;
            tab.visible_step = tab.proc.current_step;
            tab.should_adjust_step_list_scrolling = 1;

            if (tab.proc.current_step >= int(tab.proc.steps.size()))
            {
                tab.proc.current_step--;
                Program::Exit();
            }
        };

        { // Tabs
            ImGui::Spacing();

            int old_frame_border_size = ImGui::GetStyle().FrameBorderSize;

            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0); // Disable border around the button that opens the dropdown tab list.
            FINALLY( ImGui::PopStyleVar(); )

            int tab_bar_flags = ImGuiTabBarFlags_AutoSelectNewTabs | ImGuiTabBarFlags_FittingPolicyScroll | ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_NoTooltip;
            if (tabs.size() > 0)
                tab_bar_flags |= ImGuiTabBarFlags_TabListPopupButton;

            bool have_tabs = tabs.size() > 0; // We save this to a variable to avoid potential jitter when closing the last tab.
            if (ImGui::BeginTabBar("tabs", tab_bar_flags))
            {
                FINALLY( ImGui::EndTabBar(); )

                for (size_t i = 0; i < tabs.size(); i++)
                {
                    Tab &tab = tabs[i];

                    bool keep_tab_open = 1;

                    if (ImGui::BeginTabItem(Str(Data::EscapeStringForWidgetName(tab.pretty_name), "###tab:", tab.id).c_str(), &keep_tab_open))
                    {
                        FINALLY( ImGui::EndTabItem(); )

                        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, old_frame_border_size);
                        FINALLY( ImGui::PopStyleVar(); )

                        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetStyle().ItemSpacing.y);

                        ImGui::PushStyleColor(ImGuiCol_Border, 0);
                        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0);

                        ImGui::BeginChild("###current_tab", ivec2(0), true);
                        FINALLY( ImGui::EndChild(); )

                        ImGui::PopStyleVar();
                        ImGui::PopStyleColor();

                        active_tab = i;

                        ImGui::Columns(2, "main_columns", 1);

                        if (tab.first_tick)
                        {
                            int list_column_width = 0;

                            for (const Data::ProcedureStep &step : tab.proc.steps)
                                clamp_var_min(list_column_width, ImGui::CalcTextSize(step.name.c_str()).x);

                            list_column_width += ImGui::GetStyle().ScrollbarSize + ImGui::GetStyle().FramePadding.x * 4 + ImGui::GetStyle().ItemSpacing.x;

                            ImGui::SetColumnWidth(-1, list_column_width);
                        }

                        int list_column_width = ImGui::GetColumnWidth();
                        if (list_column_width < Options::Visual::step_list_min_width_pixels)
                            ImGui::SetColumnWidth(-1, Options::Visual::step_list_min_width_pixels);
                        else if (int max_list_column_width = iround(Options::Visual::step_list_max_width_relative * ImGui::GetWindowContentRegionWidth()); list_column_width > max_list_column_width)
                            ImGui::SetColumnWidth(-1, max_list_column_width);

                        Data::ProcedureStep &current_step = tab.proc.steps[tab.visible_step];

                        { // Step list
                            ImGui::TextDisabled("%s", "Шаги");

                            ImGui::BeginChildFrame(ImGui::GetID("step_list"), ImGui::GetContentRegionAvail(), ImGuiWindowFlags_HorizontalScrollbar);

                            for (std::size_t i = 0; i < tab.proc.steps.size(); i++)
                            {
                                if (ImGui::Selectable(Data::EscapeStringForWidgetName(tab.proc.steps[i].name).c_str(), int(i) == tab.visible_step, int(i) > tab.proc.current_step ? ImGuiSelectableFlags_Disabled : 0))
                                {
                                    tab.visible_step = i;
                                }

                                if (tab.should_adjust_step_list_scrolling && int(i) == tab.visible_step)
                                {
                                    ImGui::SetScrollHereY(0.75);
                                    tab.should_adjust_step_list_scrolling = 0;
                                }
                            }

                            ImGui::EndChildFrame();
                        }

                        ImGui::NextColumn();

                        { // Current step
                            ImGui::TextUnformatted(current_step.name.c_str());

                            int bottom_panel_h = ImGui::GetFrameHeightWithSpacing();
                            ImGui::BeginChildFrame(ImGui::GetID(Str("widgets:", tab.proc.current_step).c_str()), fvec2(0, -bottom_panel_h), 1);

                            int widget_index = 0;
                            for (Widgets::Widget &widget : current_step.widgets)
                            {
                                widget->Display(widget_index++, tab.proc.current_step == tab.visible_step);
                                ImGui::Spacing(); // The gui looks better with spacing after each widget including the last one.
                            }

                            ImGui::EndChildFrame();

                            if (tab.visible_step == tab.proc.current_step && ImGui::Button("Завершить шаг"))
                            {
                                if (current_step.confirm && *current_step.confirm)
                                    need_step_end_confirmation = 1;
                                else
                                    EndStep();
                            }
                        }

                        tab.first_tick = 0;
                    }

                    if (!keep_tab_open)
                    {
                        tabs.erase(tabs.begin() + i);
                        i--;
                    }
                }
            }

            if (!have_tabs)
            {
                ImGui::Indent();
                ImGui::TextDisabled("%s", "Нет открытых файлов");
                ImGui::Unindent();
            }
        }

        image_viewer.Display();
        file_selector.Display();

        { // Modal: "Are you sure you want to end the step?"
            if (need_step_end_confirmation)
                ImGui::OpenPopup("end_step_modal");

            if (ImGui::BeginPopupModal("end_step_modal", 0, Options::Visual::modal_window_flags))
            {
                ImGui::TextUnformatted("Действительно завершить шаг?");
                if (ImGui::Button("Завершить"))
                {
                    EndStep();
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Отмена") || Input::Button(Input::escape).pressed())
                {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }

        { // Modal: "Are you sure you want to exit?"
            auto Exit = [&]
            {
                // Data::ReflectedObjectToJsonFile(proc, current_file_name);
                Program::Exit();
            };

            if (exit_requested)
            {
                exit_requested = 0;

                bool need_confirmation = 0;
                for (const auto &tab : tabs)
                {
                    if (tab.proc.confirm_exit.value_or(0))
                    {
                        need_confirmation = 1;
                        break;
                    }
                }

                if (need_confirmation)
                    ImGui::OpenPopup("confirm_exit_modal");
                else
                    Exit();
            }

            if (ImGui::BeginPopupModal("confirm_exit_modal", 0, Options::Visual::modal_window_flags))
            {
                ImGui::TextUnformatted("Прервать действие и выйти?");
                if (ImGui::Button("Выйти"))
                {
                    Exit();
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Отмена") || Input::Button(Input::escape).pressed())
                {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }

        ImGui::End();

        // ImGui::ShowDemoWindow();
        // ImGui::ShowStyleEditor();
    }
};

#if IsNotOnPlatform(LINUX)
#define main SDL_main
#endif

int main(int argc, char **argv)
{
    std::string asset_dir;
    if (argc > 0)
    {
        fs::path parent_path = fs::path(argv[0]).parent_path();
        if (!parent_path.empty())
            asset_dir = parent_path.generic_string() + '/';
    }

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
            auto config = adjust(Interface::ImGuiController::Config{}, store_state_in_file = "");
            gui_controller = Interface::ImGuiController(Poly::derived<Interface::ImGuiController::GraphicsBackend_FixedFunction>, config);

            IMGUI_CHECKVERSION();
            ImGuiIO& io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

            ImFontGlyphRangesBuilder glyph_ranges_builder;
            glyph_ranges_builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
            glyph_ranges_builder.AddRanges(io.Fonts->GetGlyphRangesCyrillic());
            glyph_ranges_builder.AddText(Data::zero_width_space.c_str());
            ImVector<ImWchar> glyph_ranges;
            glyph_ranges_builder.BuildRanges(&glyph_ranges);

            std::string font_filename = asset_dir + "assets/Roboto-Regular.ttf";
            if (!std::filesystem::exists(font_filename))
                Program::Error("Font file `", font_filename, "` is missing.");
            if (!io.Fonts->AddFontFromFileTTF(font_filename.c_str(), 18.0f, 0, glyph_ranges.begin()))
                Program::Error("Unable to load font `", font_filename, "`.");

            ImGuiFreeType::BuildFontAtlas(io.Fonts, ImGuiFreeType::MonoHinting);

            Options::Visual::GuiStyle(ImGui::GetStyle());

            ImGui_ImplSDL2_InitForOpenGL(window.Handle(), window.Context());
            ImGui_ImplOpenGL2_Init();
        }
    }

    Graphics::SetClearColor(fvec3(1));

    Poly::Storage<State> state;
    auto &new_state = state.assign<StateMain>();

    if (argc > 1)
    {
        for (int i = 1; i < argc; i++)
            new_state.Load(argv[i]);
    }


    constexpr double target_frame_duration = 1 / 60.;

    while (1)
    {
        uint64_t frame_start = Clock::Time();

        window.ProcessEvents({gui_controller.EventHook(Interface::ImGuiController::pass_events)});
        if (window.Resized())
            Graphics::Viewport(window.Size());
        if (window.ExitRequested())
            state->exit_requested = 1;

        gui_controller.PreTick();
        state->Tick();

        gui_controller.PreRender();
        Graphics::Clear();
        gui_controller.PostRender();
        window.SwapBuffers();

        double delta = Clock::TicksToSeconds(Clock::Time() - frame_start);
        if (target_frame_duration > delta)
            Clock::WaitSeconds(target_frame_duration - delta);
    }
}
