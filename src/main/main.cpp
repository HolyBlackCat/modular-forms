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

        bool now_previewing_template = false; // Only makes sense for templates.

        int step_insertion_pos = -1; // Only makes sense for templates.
        int step_deletion_pos = -1; // Only makes sense for templates.
        int step_swap_pos = -1; // Only makes sense for templates.

        int widget_insertion_pos = 0; // Only makes sense for templates. Initial value doesn't really matter.
        int widget_deletion_pos = -1; // Only makes sense for templates.
        int widget_swap_pos = -1; // Only makes sense for templates.

        inline static unsigned int id_counter = 1;
        int id = id_counter++;

        // Uses `IsTemplate` to auto-insert extension if missing.
        void AssignPath(fs::path new_path)
        {
            if (!new_path.has_extension())
                new_path.replace_extension(IsTemplate() ? Options::template_extension : Options::report_extension);

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
        // tabs.erase(std::remove_if(tabs.begin(), tabs.end(), [&](const Tab &tab) {return tab.path == new_tab.path;}), tabs.end());
        return tabs.emplace_back(std::move(new_tab));
    }

    static Tab CreateTab(fs::path path, bool expect_template, bool create_new = false)
    {
        Tab new_tab;

        // Parse.
        if (create_new)
        {
            new_tab.proc.name = "Процедура";
            new_tab.proc.steps.emplace_back();
            new_tab.proc.steps.back().name = "Первый шаг процедуры";
            new_tab.proc.current_step = expect_template ? -1 : 0;
        }
        else
        {
            Stream::Input input_stream(path.string());
            new_tab.proc = Refl::FromString<Data::Procedure>(input_stream);
        }

        // Validate data.
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

        // Initialize widgets.
        // Note that this has to be done after setting the resource directory.
        if (!new_tab.IsTemplate())
            Widgets::InitializeWidgets(new_tab.proc);

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

            Tab new_tab = CreateTab(template_path, 1);
            new_tab.proc.current_step = 0;
            new_tab.AssignPath(report_path);
            Widgets::InitializeWidgets(new_tab.proc); // This is not done automatically for tabs loaded as templates.

            AddTab(std::move(new_tab));
        }
        catch (std::exception &e)
        {
            Interface::MessageBox(Interface::MessageBoxType::error, "Error", Str("Can't load `", template_path.string(), "`:\n", e.what()));
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

            Tab new_tab = CreateTab(path, path.extension() == Options::template_extension);

            AddTab(std::move(new_tab));
        }
        catch (std::exception &e)
        {
            Interface::MessageBox(Interface::MessageBoxType::warning, "Error", Str("Can't load `", path.string(), "`:\n", e.what()));
        }
    }

    void Tab_MakeTemplate(fs::path path)
    {
        if (path.empty())
            return;

        try
        {
            path = fs::weakly_canonical(path);

            // Do this first, to normalize the path.
            Tab new_tab = CreateTab(path, true, true);

            // Validate template_path.
            if (new_tab.path.extension() != Options::template_extension)
                Program::Error("Invalid extension, expected `", Options::template_extension, "`.");

            AddTab(std::move(new_tab));
        }
        catch (std::exception &e)
        {
            Interface::MessageBox(Interface::MessageBoxType::error, "Error", Str("Can't create `", path.string(), "`:\n", e.what()));
        }
    }

    bool Tab_Save() // Returns `true` on success.
    {
        if (!HaveActiveTab())
            return 0;

        try
        {
            Stream::Output output_stream(tabs[active_tab].path.string());
            Refl::ToString(tabs[active_tab].proc, output_stream, Refl::ToStringOptions::Pretty());
            output_stream.Flush();
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

                    if (ImGui::MenuItem("Новый шаблон"))
                    {
                        if (auto result = FileDialogs::SaveTemplate())
                            Tab_MakeTemplate(*result);
                    }

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
                                got_path = false;
                        }

                        if (!got_path)
                        {
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

            { // Write to file
                Stream::Output output_stream(tab.path.string());
                Refl::ToString(tab.proc, output_stream, Refl::ToStringOptions::Pretty());
            }

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

                auto CloseTab = [&](int tab_index)
                {
                    int old_active_tab = active_tab; // Just in case.
                    FINALLY( active_tab = old_active_tab; )
                    active_tab = tab_index;

                    Tab_Save();

                    tabs.erase(tabs.begin() + tab_index);
                };

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
                            if (tab.IsTemplate() && !tab.now_previewing_template)
                            {
                                ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
                                FINALLY( ImGui::PopItemWidth(); );

                                ImGui::TextUnformatted("Название процедуры");
                                ImGui::InputText("###proc_name_input", &tab.proc.name);


                                if (ImGui::SmallButton("Список библиотек"))
                                    ImGui::OpenPopup("library_editor_modal");

                                if (ImGui::IsPopupOpen("library_editor_modal"))
                                {
                                    ImGui::SetNextWindowPos(window.Size() / 2, ImGuiCond_Always, fvec2(0.5));
                                    ImGui::SetNextWindowSize(window.Size() - 48);
                                    if (ImGui::BeginPopupModal("library_editor_modal", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
                                    {
                                        FINALLY( ImGui::EndPopup(); )

                                        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.4);
                                        FINALLY( ImGui::PopItemWidth(); )

                                        ImGui::TextUnformatted("Редактирование библиотек");

                                        static constexpr const char *close_button_text = "Закрыть";
                                        static const int close_button_text_w = ImGui::CalcTextSize(close_button_text).x;

                                        ImGui::SameLine();
                                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - close_button_text_w - ImGui::GetStyle().FramePadding.x * 2);

                                        if (ImGui::Button("Закрыть"))
                                            ImGui::CloseCurrentPopup();

                                        ImGui::Separator();

                                        int lib_index = 0, del_lib_index = -1;
                                        for (Data::Library &lib : tab.proc.libraries)
                                        {
                                            ImGui::InputText("ID###libname:{}"_format(lib_index).c_str(), &lib.id);
                                            ImGui::InputText("Файл (без расширения)###libfile:{}"_format(lib_index).c_str(), &lib.file);

                                            if (ImGui::SmallButton("Удалить###libfuncdel:{}"_format(lib_index).c_str()))
                                                del_lib_index = lib_index;

                                            if (ImGui::CollapsingHeader("Список функций:###libfunclist:{}"_format(lib_index).c_str()))
                                            {
                                                ImGui::Indent();
                                                FINALLY( ImGui::Unindent(); )

                                                ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.3);
                                                FINALLY( ImGui::PopItemWidth(); )

                                                ImGui::TextUnformatted("Функции:");

                                                int func_index = 0, del_func_index = -1;
                                                for (Data::LibraryFunc &func : lib.functions)
                                                {
                                                    ImGui::InputText("ID###libfunclib:{}:{}"_format(lib_index, func_index).c_str(), &func.id);
                                                    ImGui::InputText("Имя в библиотеке###libfunclib:{}:{}"_format(lib_index, func_index).c_str(), &func.name);
                                                    if (ImGui::SmallButton("Удалить###libfuncdel:{}:{}"_format(lib_index, func_index).c_str()))
                                                        del_func_index = func_index;

                                                    ImGui::Spacing();
                                                    ImGui::Spacing();

                                                    func_index++;
                                                }

                                                if (del_func_index != -1)
                                                    lib.functions.erase(lib.functions.begin() + del_func_index);

                                                if (ImGui::Button("+"))
                                                    lib.functions.emplace_back();
                                            }

                                            ImGui::Spacing();
                                            ImGui::Separator();
                                            ImGui::Spacing();

                                            lib_index++;
                                        }

                                        if (del_lib_index != -1)
                                            tab.proc.libraries.erase(tab.proc.libraries.begin() + del_lib_index);

                                        if (ImGui::Button("+"))
                                            tab.proc.libraries.emplace_back();
                                    }
                                }

                                ImGui::Checkbox("Спрашивать\nпри закрытии", &tab.proc.confirm_exit);

                                ImGui::Spacing();
                            }

                            ImGui::TextDisabled("%s", "Шаги");

                            if (tab.proc.IsTemplate() && !tab.now_previewing_template)
                            {
                                static constexpr const char *button_add = "Добавить";
                                static const int button_add_w = ImGui::CalcTextSize(button_add).x;

                                ImGui::SameLine();
                                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - button_add_w - ImGui::GetStyle().FramePadding.x * 2);
                                if (ImGui::SmallButton(button_add))
                                    tab.step_insertion_pos = tab.visible_step + 1;
                            }

                            ImGui::BeginChildFrame(ImGui::GetID("step_list:{}"_format(tab.visible_step).c_str()), ImGui::GetContentRegionAvail(), ImGuiWindowFlags_HorizontalScrollbar);

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
                            // Switch between editing and preview
                            if (tab.IsTemplate())
                            {
                                const char *text = tab.now_previewing_template ? "Редактирование" : "Предпросмотр";

                                if (ImGui::SmallButton(text))
                                {
                                    try
                                    {
                                        if (!tab.now_previewing_template)
                                        {
                                            Widgets::InitializeWidgets(tab.proc);
                                        }

                                        tab.now_previewing_template = !tab.now_previewing_template;
                                    }
                                    catch (std::exception &e)
                                    {
                                        Interface::MessageBox(Interface::MessageBoxType::error, "Error", "Unable to preview:\n{}"_format(e.what()));
                                    }
                                }
                            }

                            // Step manipulation options
                            if (tab.proc.steps.size() > 1/*sic*/ && tab.IsTemplate() && !tab.now_previewing_template)
                            {
                                static constexpr const char *button_up = "Вверх", *button_down = "Вниз", *button_delete = "X";
                                static const int button_sum_w = ImGui::CalcTextSize(button_up).x + ImGui::CalcTextSize(button_down).x + ImGui::CalcTextSize(button_delete).x
                                    + ImGui::GetStyle().FramePadding.x * 6 + ImGui::GetStyle().ItemSpacing.x * 2;

                                ImGui::SameLine();
                                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - button_sum_w);

                                if (ImGui::SmallButton(button_up))
                                    tab.step_swap_pos = tab.visible_step-1;
                                ImGui::SameLine();
                                if (ImGui::SmallButton(button_down))
                                    tab.step_swap_pos = tab.visible_step;
                                ImGui::SameLine();
                                if (ImGui::SmallButton(button_delete))
                                    tab.step_deletion_pos = tab.visible_step;
                            }

                            // Step name (and some config)
                            if (tab.proc.IsTemplate() && !tab.now_previewing_template)
                            {
                                ImGui::TextUnformatted("Название шага");

                                ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.35);
                                FINALLY( ImGui::PopItemWidth(); )

                                ImGui::InputText("###step_name", &current_step.name);

                                ImGui::SameLine();
                                ImGui::Checkbox("Требовать подтверждения шага", &current_step.confirm);
                            }
                            else
                            {
                                ImGui::TextUnformatted(current_step.name.c_str());
                            }

                            // Otherwise the right side of the frame is slightly clipped.
                            ImGui::PushClipRect(ivec2(0), window.Size(), false);
                            FINALLY( ImGui::PopClipRect(); )

                            int bottom_panel_h = ImGui::GetFrameHeightWithSpacing();
                            // ImGui::BeginChildFrame(ImGui::GetID(Str("widgets:", tab.proc.current_step).c_str()), fvec2(0, -bottom_panel_h), 1);
                            ImGui::BeginChildFrame(ImGui::GetID(Str("widgets{}:", tab.now_previewing_template ? "" : "_editing", tab.proc.current_step).c_str()),
                                ivec2(ImGui::GetContentRegionAvail()) + ivec2(ImGui::GetStyle().WindowPadding.x, -bottom_panel_h), 1);


                            auto InsertWidgetButton = [&](int index)
                            {
                                int indent_w = ImGui::GetFrameHeight() * 2;
                                ImGui::Indent(indent_w);
                                FINALLY( ImGui::Unindent(indent_w); )

                                static constexpr const char *text = "Вставить виджет";
                                if (ImGui::SmallButton("{}###insert_widget:{}"_format(text, index).c_str()))
                                {
                                    tab.widget_insertion_pos = index;
                                    ImGui::OpenPopup("new_widget");
                                }
                            };


                            // Widget list.
                            int widget_index = 0;
                            for (Widgets::Widget &widget : current_step.widgets)
                            {
                                if (tab.IsTemplate() && !tab.now_previewing_template)
                                {
                                    // Customization controls

                                    if (widget_index != 0)
                                        ImGui::Separator();

                                    InsertWidgetButton(widget_index);
                                    ImGui::Separator();

                                    // Buttons to move or delete the widget

                                    ivec2 name_cursor_pos = ImGui::GetCursorPos();

                                    static constexpr const char *button_up = "Вверх", *button_down = "Вниз", *button_delete = "X";
                                    static const int button_total_w = ImGui::CalcTextSize(button_up).x + ImGui::CalcTextSize(button_down).x + ImGui::CalcTextSize(button_delete).x
                                        + ImGui::GetStyle().FramePadding.x * 6 + ImGui::GetStyle().ItemSpacing.x * 2;

                                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - button_total_w);

                                    if (ImGui::SmallButton("{}###move_widget_up:{}"_format(button_up, widget_index).c_str()))
                                        tab.widget_swap_pos = widget_index - 1;

                                    ImGui::SameLine();
                                    if (ImGui::SmallButton("{}###move_widget_down:{}"_format(button_down, widget_index).c_str()))
                                        tab.widget_swap_pos = widget_index;
                                    ImGui::SameLine();
                                    if (ImGui::SmallButton("{}###move_widget_delete:{}"_format(button_delete, widget_index).c_str()))
                                        tab.widget_deletion_pos = widget_index;

                                    // Name
                                    ImGui::SetCursorPos(name_cursor_pos);
                                    ImGui::TextUnformatted(widget->PrettyName().c_str());

                                    if (widget->IsEditable())
                                    {
                                        int indent_w = ImGui::GetFrameHeight() * 1;
                                        ImGui::Indent(indent_w);
                                        FINALLY( ImGui::Unindent(indent_w); )

                                        if (ImGui::CollapsingHeader("{}###widget_collapsing_header:{}"_format("Редактировать", widget_index).c_str()))
                                        {
                                            widget->DisplayEditor(tab.proc, widget_index);
                                        }
                                    }
                                }
                                else
                                {
                                    // Render widget normally.
                                    widget->Display(widget_index, tab.proc.current_step == tab.visible_step && !tab.IsTemplate());
                                    ImGui::Spacing(); // The gui looks better with spacing after each widget including the last one.
                                }

                                widget_index++;
                            }

                            // "Insert new widget" button after the last widget.
                            if (tab.IsTemplate() && !tab.now_previewing_template)
                            {
                                ImGui::Separator();
                                InsertWidgetButton(current_step.widgets.size());
                            }
                            // "Insert new widget" popup
                            if (ImGui::IsPopupOpen("new_widget"))
                            {
                                constexpr const char *popup_title = "Добавление нового виджета";
                                static const int popup_title_w = ImGui::CalcTextSize(popup_title).x;

                                ImGui::SetNextWindowSize(ivec2(popup_title_w * 2, ImGui::GetFrameHeight() * 12), ImGuiCond_Always);
                                ImGui::SetNextWindowPos(window.Size() / 2, ImGuiCond_Always, fvec2(0.5));

                                if (ImGui::BeginPopupModal("new_widget", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar))
                                {
                                    FINALLY( ImGui::EndPopup(); )

                                    if (!tab.IsTemplate() || tab.now_previewing_template)
                                    {
                                        ImGui::CloseCurrentPopup();
                                    }
                                    else
                                    {
                                        ImGui::TextDisabled("%s", "Добавление нового виджета");
                                        ImGui::SameLine();

                                        constexpr const char *text_cancel = "Отмена";
                                        ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - ImGui::CalcTextSize(text_cancel).x - ImGui::GetStyle().FramePadding.x * 2);
                                        if (ImGui::SmallButton(text_cancel))
                                            ImGui::CloseCurrentPopup();

                                        ImGui::Separator();

                                        struct Entry
                                        {
                                            std::string name;
                                            int internal_index = 0;
                                        };
                                        static const std::vector<Entry> menu_entries = []{
                                            std::vector<Entry> menu_entries;
                                            size_t count = Refl::Polymorphic::DerivedClassCount<Widgets::BasicWidget>();
                                            for (size_t i = 0; i < count; i++)
                                            {
                                                Entry &new_entry = menu_entries.emplace_back();
                                                new_entry.name = Refl::Polymorphic::ConstructFromIndex<Widgets::BasicWidget>(i)->PrettyName();
                                                new_entry.internal_index = i;
                                            }
                                            std::sort(menu_entries.begin(), menu_entries.end(), [](const Entry &a, const Entry &b){return a.name < b.name;});
                                            return menu_entries;
                                        }();

                                        ImGui::BeginChild(42, ImGui::GetContentRegionAvail());
                                        FINALLY( ImGui::EndChild(); )

                                        for (const Entry &menu_entry : menu_entries)
                                        {
                                            if (ImGui::Selectable(menu_entry.name.c_str(), false, ImGuiSelectableFlags_DontClosePopups))
                                            {
                                                clamp_var(tab.widget_insertion_pos, 0, int(current_step.widgets.size()));
                                                current_step.widgets.insert(current_step.widgets.begin() + tab.widget_insertion_pos,
                                                    Refl::Polymorphic::ConstructFromIndex<Widgets::BasicWidget>(menu_entry.internal_index));
                                                ImGui::CloseCurrentPopup();
                                            }
                                        }
                                    }
                                }
                            }
                            // Widget deletion and movement
                            if (tab.IsTemplate() && !tab.now_previewing_template)
                            {
                                if (tab.widget_deletion_pos != -1)
                                {
                                    if (tab.widget_deletion_pos >= 0 && tab.widget_deletion_pos < int(current_step.widgets.size()))
                                        current_step.widgets.erase(current_step.widgets.begin() + tab.widget_deletion_pos);

                                    tab.widget_deletion_pos = -1;

                                    // For good measure.
                                    tab.widget_insertion_pos = -1;
                                    tab.widget_swap_pos = -1;
                                }

                                if (tab.widget_swap_pos != -1)
                                {
                                    if (tab.widget_swap_pos >= 0 && tab.widget_swap_pos + 1 < int(current_step.widgets.size()))
                                        std::swap(current_step.widgets[tab.widget_swap_pos], current_step.widgets[tab.widget_swap_pos+1]);

                                    tab.widget_swap_pos = -1;

                                    // For good measure.
                                    tab.widget_insertion_pos = -1;
                                    tab.widget_deletion_pos = -1;
                                }
                            }

                            // Step insertion, deletion and movement
                            if (tab.IsTemplate() && !tab.now_previewing_template)
                            {
                                if (tab.step_insertion_pos != -1)
                                {
                                    if (tab.step_insertion_pos >= 0 && tab.step_insertion_pos <=/*sic*/ int(tab.proc.steps.size()))
                                    {
                                        tab.proc.steps.emplace(tab.proc.steps.begin() + tab.step_insertion_pos);
                                        tab.visible_step = tab.step_insertion_pos;
                                    }

                                    tab.step_insertion_pos = -1;

                                    // For good measure.
                                    tab.step_deletion_pos = -1;
                                    tab.step_swap_pos = -1;
                                }

                                if (tab.step_deletion_pos != -1)
                                {
                                    if (tab.step_deletion_pos >= 0 && tab.step_deletion_pos < int(tab.proc.steps.size()))
                                    {
                                        tab.proc.steps.erase(tab.proc.steps.begin() + tab.step_deletion_pos);
                                        clamp_var(tab.visible_step, 0, int(tab.proc.steps.size()));
                                    }

                                    tab.step_deletion_pos = -1;

                                    // For good measure.
                                    tab.step_insertion_pos = -1;
                                    tab.step_swap_pos = -1;
                                }

                                if (tab.step_swap_pos != -1)
                                {
                                    if (tab.step_swap_pos >= 0 && tab.step_swap_pos + 1 < int(tab.proc.steps.size()))
                                    {
                                        std::swap(tab.proc.steps[tab.step_swap_pos], tab.proc.steps[tab.step_swap_pos+1]);
                                        if (tab.visible_step == tab.step_swap_pos)
                                        {
                                            tab.visible_step++;
                                            tab.should_adjust_step_list_scrolling = true;
                                        }
                                        else if (tab.visible_step == tab.step_swap_pos + 1)
                                        {
                                            tab.visible_step--;
                                            tab.should_adjust_step_list_scrolling = true;
                                        }
                                    }

                                    tab.step_swap_pos = -1;

                                    // For good measure.
                                    tab.step_insertion_pos = -1;
                                    tab.step_deletion_pos = -1;
                                }
                            }

                            ImGui::EndChildFrame();

                            if (tab.visible_step == tab.proc.current_step && ImGui::Button("Завершить шаг"))
                            {
                                if (current_step.confirm)
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
                        if (tab.proc.confirm_exit && !tab.IsTemplate())
                        {
                            ImGui::OpenPopup("confirm_closing_tab_modal");
                        }
                        else
                        {
                            CloseTab(i);
                            i--;
                        }
                    }
                }

                // Tab close confirmation.
                if (ImGui::BeginPopupModal("confirm_closing_tab_modal", 0, Options::Visual::modal_window_flags))
                {
                    ImGui::TextUnformatted("Закрыть вкладку?");
                    if (ImGui::Button("Закрыть"))
                    {
                        CloseTab(active_tab);
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
                    if (tab.proc.confirm_exit && !tab.IsTemplate())
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
