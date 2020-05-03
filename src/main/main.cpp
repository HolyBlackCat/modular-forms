#include <iostream>

#include "main/common.h"
#include "main/file_dialogs.h"
#include "main/gui_strings.h"
#include "main/image_viewer.h"
#include "main/options.h"
#include "main/procedure_data.h"
#include "main/widgets.h"

namespace fs = std::filesystem;

constexpr const char *program_name = "Modular forms";

Interface::Window window;
Interface::ImGuiController gui_controller;
Input::Mouse mouse;

fs::path program_directory;

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
        fs::path path; // Don't modify this directly. Use `AssignPath()`.
        bool first_tick = 1;

        int visible_step = 0;
        bool should_adjust_step_list_scrolling = 0;

        inline static unsigned int id_counter = 1;
        int id = id_counter++;

        void AssignPath(fs::path new_path)
        {
            path = fs::weakly_canonical(new_path);
            pretty_name = path.stem().string(); // `stem` means file name without extension.
        }

        bool IsFinished() const
        {
            return proc.current_step >= int(proc.steps.size());
        }

        bool IsTemplate() const
        {
            return proc.current_step == -1;
        }
    };

    std::vector<Tab> tabs; // Don't modify directly. Use `AddTab()`.
    int active_tab = -1;

    bool HaveActiveTab() const {return active_tab >= 0 && active_tab < int(tabs.size());}

    GuiElements::ImageViewer image_viewer;

    StateMain() {}

    Tab& AddTab(Tab new_tab)
    {
        tabs.erase(std::remove_if(tabs.begin(), tabs.end(), [&](const Tab &tab) {return tab.path == new_tab.path;}), tabs.end());
        return tabs.emplace_back(std::move(new_tab));
    }

    static Tab CreateTab(const char *json_data, bool expect_template, fs::path path)
    {
        Tab new_tab;

        // Parse JSON.
        Json json(json_data, 64);
        new_tab.proc = Widgets::ReflectedObjectFromJson<Data::Procedure>(json.GetView());

        // Validate JSON data.
        if (new_tab.proc.steps.size() < 1)
            Program::Error("The procedure must have at least one step.");
        if (expect_template)
        {
            if (new_tab.proc.current_step != -1)
                Program::Error("Invalid current step index.");
        }
        else
        {
            if (new_tab.proc.current_step < 0 || new_tab.proc.current_step > int(new_tab.proc.steps.size())) // Sic.
                Program::Error("Current step index is out of range.");
        }

        // Set resource directory.
        new_tab.proc.resource_dir = program_directory / Options::template_dir;

        // Load dynamic libraries.
        if (new_tab.proc.libraries)
        {
            for (Data::Library &lib : *new_tab.proc.libraries)
            {
                constexpr const char *lib_ext = (PLATFORM_IS(windows) ? ".dll" : ".so");

                lib.library = SharedLibrary((new_tab.proc.resource_dir / (lib.file + lib_ext)).string());

                for (Data::LibraryFunc &func : lib.functions)
                {
                    if (func.optional.value_or(false))
                        func.ptr = reinterpret_cast<Data::external_func_ptr_t>(lib.library.GetFunctionOrNull(func.name));
                    else
                        func.ptr = reinterpret_cast<Data::external_func_ptr_t>(lib.library.GetFunction(func.name));
                }
            }
        }

        // Initialize widgets.
        // Note that this has to be done after setting the resource directory.
        Widgets::InitializeWidgetsRecursively(new_tab.proc, new_tab.proc);

        // Set the visible step.
        new_tab.visible_step = 0;
        // new_tab.visible_step = clamp_max(new_tab.proc.current_step, int(new_tab.proc.steps.size())-1);

        // Save path.
        new_tab.AssignPath(path);

        return new_tab;
    }

    void Tab_MakeReportFromTemplate(fs::path template_path, fs::path report_path)
    {
        if (template_path.empty() || report_path.empty())
            return;

        try
        {
            template_path = fs::weakly_canonical(template_path);

            // Validate template_path.
            fs::file_status status = fs::status(template_path);
            if (!fs::exists(status))
                Program::Error("File doesn't exist.");
            if (!fs::is_regular_file(status))
                Program::Error("Not a regular file.");
            if (template_path.extension() != Options::template_extension)
                Program::Error("Invalid extension, expected `", Options::template_extension, "`.");

            Tab &new_tab = AddTab(CreateTab(Stream::ReadOnlyData(template_path.string()).string(), 1, report_path));
            new_tab.proc.current_step = 0;
        }
        catch (std::exception &e)
        {
            Interface::MessageBox(Interface::MessageBoxType::warning, "Error", Str("Can't load `", template_path.string(), "`:\n", e.what()));
        }
    }

    void Tab_LoadReportOrTemplate(fs::path path)
    {
        if (path.empty())
            return;

        try
        {
            path = fs::weakly_canonical(path);

            // Validate path.
            fs::file_status status = fs::status(path);
            if (!fs::exists(status))
                Program::Error("File doesn't exist.");
            if (!fs::is_regular_file(status))
                Program::Error("Not a regular file.");
            if (path.extension() != Options::report_extension && path.extension() != Options::template_extension)
                Program::Error("Invalid extension, expected `", Options::report_extension, "` or `", Options::template_extension, "`.");

            Tab new_tab = CreateTab(Stream::ReadOnlyData(path.string()).string(), path.extension() == Options::template_extension, path);

            new_tab.AssignPath(path);

            AddTab(std::move(new_tab));
        }
        catch (std::exception &e)
        {
            Interface::MessageBox(Interface::MessageBoxType::warning, "Error", Str("Can't load `", path.string(), "`:\n", e.what()));
        }
    }

    bool Tab_Save() // Returns `true` on success.
    {
        if (!HaveActiveTab())
            return 0;

        try
        {
            Widgets::ReflectedObjectToJsonFile(tabs[active_tab].proc, tabs[active_tab].path.string());
            return 1;
        }
        catch (std::exception &e)
        {
            Interface::MessageBox(Interface::MessageBoxType::warning, "Error", "Unable to save `{}`:\n{}"_format(tabs[active_tab].path.string(), e.what()));
            return 0;
        }
    }

    void Tick() override
    {
        for (const std::string &new_file : window.DroppedFiles())
            Tab_LoadReportOrTemplate(new_file);

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
                    if (ImGui::MenuItem("Новый отчет на основе шаблона"))
                    {
                        if (auto result_template = FileDialogs::OpenTemplate())
                            if (auto result_report = FileDialogs::SaveReport())
                                Tab_MakeReportFromTemplate(*result_template, *result_report);
                    }

                    if (ImGui::MenuItem("Новый шаблон", nullptr, nullptr, 0)) {}

                    ImGui::Separator();

                    if (ImGui::MenuItem("Открыть отчет"))
                    {
                        if (auto result = FileDialogs::OpenReport())
                            Tab_LoadReportOrTemplate(*result);
                    }

                    if (ImGui::MenuItem("Открыть шаблон"))
                    {
                        if (auto result = FileDialogs::OpenTemplate())
                            Tab_LoadReportOrTemplate(*result);
                    }

                    ImGui::Separator();

                    if (ImGui::MenuItem("Сохранить", nullptr, nullptr, HaveActiveTab() && tabs[active_tab].IsTemplate()))
                    {
                        Tab_Save();
                    }

                    if (ImGui::IsItemHovered() && HaveActiveTab() && !tabs[active_tab].IsTemplate())
                    {
                        ImGui::SetTooltip("%s", "Файлы отчетов сохраняются автоматически.");
                    }

                    if (ImGui::MenuItem("Сохранить как", nullptr, nullptr, HaveActiveTab()))
                    {
                        bool got_path = 0;
                        fs::path old_path;

                        if (tabs[active_tab].IsTemplate())
                        {
                            if (auto result = FileDialogs::SaveTemplate())
                            {
                                old_path = tabs[active_tab].path;
                                tabs[active_tab].AssignPath(std::move(*result));
                                got_path = 1;
                            }
                        }
                        else
                        {
                            if (auto result = FileDialogs::SaveReport())
                            {
                                old_path = tabs[active_tab].path;
                                tabs[active_tab].AssignPath(std::move(*result));
                                got_path = 1;
                            }
                        }

                        if (got_path)
                        {
                            bool ok = Tab_Save();
                            if (!ok)
                                tabs[active_tab].AssignPath(std::move(old_path));
                        }
                    }

                    ImGui::Separator();

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

            if (tab.IsFinished())
                return;

            tab.proc.current_step++;

            Widgets::ReflectedObjectToJsonFile(tab.proc, tab.path.string());

            if (tab.IsFinished()) // Sic!
                return;

            tab.visible_step = tab.proc.current_step;
            tab.should_adjust_step_list_scrolling = 1;
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

                    // The current tab
                    if (ImGui::BeginTabItem(Str(Data::EscapeStringForWidgetName(tab.pretty_name), "###tab:", tab.id).c_str(), &keep_tab_open))
                    {
                        FINALLY( ImGui::EndTabItem(); )

                        window.SetTitle("{}{} - {} - {}"_format(tab.proc.name, tab.proc.IsTemplate() ? " [шаблон]" : "", tab.path.string(), program_name));

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
                                clamp_var_min(list_column_width, int(ImGui::CalcTextSize(step.name.c_str()).x));

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
                                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(int(i) > tab.proc.current_step && !tab.IsTemplate() ? ImGuiCol_TextDisabled : ImGuiCol_Text));
                                FINALLY( ImGui::PopStyleColor(); )

                                if (ImGui::Selectable(Data::EscapeStringForWidgetName(tab.proc.steps[i].name).c_str(), int(i) == tab.visible_step))
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

                            // Otherwise the right side of the frame is slightly clipped.
                            ImGui::PushClipRect(ivec2(0), window.Size(), false);
                            FINALLY( ImGui::PopClipRect(); )

                            int bottom_panel_h = ImGui::GetFrameHeightWithSpacing();
                            // ImGui::BeginChildFrame(ImGui::GetID(Str("widgets:", tab.proc.current_step).c_str()), fvec2(0, -bottom_panel_h), 1);
                            ImGui::BeginChildFrame(ImGui::GetID(Str("widgets:", tab.proc.current_step).c_str()), ivec2(ImGui::GetContentRegionAvail()) + ivec2(ImGui::GetStyle().WindowPadding.x, -bottom_panel_h), 1);

                            int widget_index = 0;
                            for (Widgets::Widget &widget : current_step.widgets)
                            {
                                widget->Display(widget_index++, tab.proc.current_step == tab.visible_step || tab.IsTemplate());
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

                            if (tab.visible_step != tab.proc.current_step)
                            {
                                if (tab.IsTemplate())
                                {
                                    // Nothing.
                                }
                                else if (tab.IsFinished())
                                {
                                    ImGui::TextUnformatted("Процедура завершена.");
                                }
                                else
                                {
                                    ImGui::TextUnformatted(tab.visible_step < tab.proc.current_step ? "Этот шаг уже завершен." : "Этот шаг еще не начат.");
                                    ImGui::SameLine();
                                    if (ImGui::SmallButton("Показать текущий шаг"))
                                    {
                                        tab.visible_step = tab.proc.current_step;
                                        tab.should_adjust_step_list_scrolling = 1;
                                    }
                                }
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
                window.SetTitle(program_name);

                ImGui::Indent();
                ImGui::TextDisabled("%s", "Нет открытых файлов");
                ImGui::Unindent();
            }
        }

        image_viewer.Display();

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
                for (size_t i = 0; i < tabs.size(); i++)
                {
                    active_tab = i;
                    Tab_Save();
                }
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

int _main_(int argc, char **argv)
{
    if (argc > 0)
    {
        fs::path parent_path = fs::path(argv[0]).parent_path();
        if (!parent_path.empty())
            program_directory = parent_path.generic_string();
    }

    { // Initialize
        { // Window
            ivec2 window_size(800, 600);

            Interface::WindowSettings window_settings;
            window_settings.min_size = window_size / 2;
            window_settings.gl_major = 2;
            window_settings.gl_minor = 1;
            window_settings.gl_profile = Interface::Profile::any;
            window_settings.vsync = Interface::VSync::disabled;

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

            std::string font_filename = (program_directory / fs::path("assets/Roboto-Regular.ttf")).string();
            if (!std::filesystem::exists(font_filename))
                Program::Error("Font file `", font_filename, "` is missing.");
            if (!io.Fonts->AddFontFromFileTTF(font_filename.c_str(), 16.0f, 0, glyph_ranges.begin()))
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
            new_state.Tab_LoadReportOrTemplate(argv[i]);
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
