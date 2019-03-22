#include <iostream>

#include <map>
#include <optional>
#include <vector>

#include "interface/window.h"
#include "graphics/complete.h"
#include "program/exit.h"
#include "utils/adjust.h"
#include "utils/clock.h"
#include "utils/dynamic_storage.h"
#include "utils/json.h"
#include "utils/mat.h"
#include "utils/meta.h"
#include "utils/metronome.h"

#include <imgui.h>
#include <imgui_freetype.h>
#include <imgui_impl_opengl2.h>
#include <imgui_impl_sdl.h>

Interface::Window window;

namespace VisualOptions
{
    constexpr float
        outer_margin = 6,
        step_list_min_width_pixels = 64,
        step_list_max_width_relative = 0.4;

    constexpr int
        modal_window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;

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
    const std::string zero_width_space = "\xEF\xBB\xBF";

    // `##` has a special meaning for ImGui when used in widget names, so we break those with zero-width spaces.
    std::string EscapeStringForWidgetName(const std::string &source_str)
    {
        std::string ret;

        for (char ch : source_str)
        {
            ret += ch;
            if (ch == '#')
                ret += zero_width_space;
        }

        return ret;
    }


    struct WidgetBase_Low
    {
        WidgetBase_Low() = default;
        WidgetBase_Low(const WidgetBase_Low &) = default;
        WidgetBase_Low(WidgetBase_Low &&) = default;
        WidgetBase_Low &operator=(const WidgetBase_Low &) = default;
        WidgetBase_Low &operator=(WidgetBase_Low &&) = default;

        virtual void Init() {};
        virtual void Display(int index) = 0;
        virtual ~WidgetBase_Low() = default;
    };

    template <typename T> T ReflectedObjectFromJson(Json::View view);

    struct WidgetInternalFunctions
    {
        using Widget = Dyn<WidgetBase_Low, WidgetInternalFunctions>;

        void (*from_json)(Dynamic::Param<WidgetBase_Low>, Json::View, std::string);

        template <typename T> constexpr void _make()
        {
            from_json = [](Dynamic::Param<WidgetBase_Low> obj, Json::View view, std::string elem_name)
            {
                if constexpr (Refl::is_reflected<T>)
                    obj.template get<T>() = ReflectedObjectFromJson<T>(view[elem_name]);
            };
        }
    };
    using Widget = WidgetInternalFunctions::Widget;

    struct WidgetFactory
    {
        template <typename T> friend class WidgetBase;

        std::map<std::string, Widget(*)()> factory_funcs;

        template <typename T> void Register(std::string name)
        {
            factory_funcs.insert({name, []{return Widget::make<T>();}});
        }

      public:
        WidgetFactory() {}

        Widget Make(std::string name) const
        {
            auto it = factory_funcs.find(name);
            if (it == factory_funcs.end())
                Program::Error("Invalid widget name: `", name, "`.");
            return it->second();
        }
    };
    WidgetFactory &widget_factory()
    {
        static WidgetFactory ret;
        return ret;
    }

    template <typename T> struct is_std_optional : std::false_type {};
    template <typename T> struct is_std_optional<std::optional<T>> : std::true_type {};

    template <typename T> T ReflectedObjectFromJson(Json::View view)
    {
        if constexpr (std::is_same_v<T, bool>)
        {
            return view.GetBool();
        }
        else if constexpr (std::is_same_v<T, std::string>)
        {
            return view.GetString();
        }
        else if constexpr (std::is_integral_v<T>)
        {
            return view.GetInt();
        }
        else if constexpr (std::is_floating_point_v<T>)
        {
            return view.GetReal();
        }
        else if constexpr (std::is_same_v<T, Widget>)
        {
            T ret = widget_factory().Make(view["type"].GetString());
            ret.funcs().from_json(ret, view, "data");
            ret->Init();
            return ret;
        }
        else if constexpr (Refl::is_reflected<T>)
        {
            T ret;

            using refl_t = Refl::Interface<T>;
            auto refl = Refl::Interface(ret);

            if constexpr (refl_t::is_structure)
            {
                refl.for_each_field([&](auto index)
                {
                    constexpr int i = index.value;
                    using field_type = typename refl_t::template field_type<i>;
                    const std::string &field_name = refl.field_name(i);
                    auto &field_ref = refl.template field_value<i>();
                    if constexpr (!is_std_optional<field_type>::value)
                    {
                        field_ref = ReflectedObjectFromJson<field_type>(view[field_name]);
                    }
                    else
                    {
                        if (view.HasElement(field_name))
                            field_ref = ReflectedObjectFromJson<typename field_type::value_type>(view[field_name]);
                        else
                            field_ref = {};
                    }
                });
            }
            else if constexpr (refl_t::is_container)
            {
                int elements = view.GetArraySize();

                for (int i = 0; i < elements; i++)
                    refl.insert(ReflectedObjectFromJson<typename refl_t::element_type>(view[i]));
            }
            else
            {
                static_assert(Meta::value<false, T>, "This type is not supported.");
            }

            return ret;
        }
        else
        {
            static_assert(Meta::value<false, T>, "This type is not supported.");
        }
    }

    // Some trickery to register widgets.
    template <typename T> struct WidgetBase_impl_0 : public WidgetBase_Low
    {
        [[maybe_unused]] inline static int dummy = []{
            widget_factory().Register<T>(T::internal_name);
            return 0;
        }();
    };
    template <typename T, int * = &WidgetBase_impl_0<T>::dummy> struct WidgetBase_impl_1 : WidgetBase_impl_0<T> {}; // Yes, this is truly necessary.
    template <typename T> class WidgetBase : public WidgetBase_impl_1<T> {};


    ReflectStruct(ProcedureStep,(
        (std::string)(name),
        (std::optional<bool>)(confirm),
        (std::vector<Widget>)(widgets),
    ))

    ReflectStruct(Procedure,(
        (std::string)(name),
        (std::optional<bool>)(confirm_exit),
        (std::vector<ProcedureStep>)(steps),
    ))
}

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

        Reflect(ButtonList)
        (
            (std::vector<std::string>)(labels),
            (std::optional<bool>)(packed),
        )

        int size_x = 0; // If `packed == true`, this is set to -1 in `Init()`, and then to the button width on the first `Display()` call. Otherwise it stays at 0.

        void Init() override
        {
            if (labels.size() == 0)
                Program::Error("A button list must contain at least one button.");

            // We can't calculate a proper button width here, as fonts don't seem to be loaded this early.
            if (packed && *packed)
                size_x = -1;
        }

        void Display(int index) override
        {
            if (size_x == -1)
            {
                for (const std::string &str : labels)
                    clamp_var_min(size_x, ImGui::CalcTextSize(str.c_str()).x);
                size_x += ImGui::GetStyle().FramePadding.x*2;
            }

            int size_with_padding = size_x + ImGui::GetStyle().ItemSpacing.x;

            int columns = size_x ? clamp_min(int(ImGui::GetWindowContentRegionWidth()) / size_with_padding, 1) : 1;
            int elems_per_column = columns != 1 ? (labels.size() + columns - 1) / columns : labels.size();
            columns = (labels.size() + elems_per_column - 1) / elems_per_column;

            ImGui::Columns(columns, 0, 0);

            int cur_y = ImGui::GetCursorPosY();

            int elem_index = 0;
            for (int i = 0; i < columns; i++)
            {
                for (int j = 0; j < elems_per_column; j++)
                {
                    if (elem_index >= int(labels.size()))
                        break; // I don't think we want to terminate the outer loop early. We want to use all columns no matter what.

                    ImGui::Button(Str(Data::EscapeStringForWidgetName(labels[elem_index]), "###", index, ":", elem_index).c_str(), fvec2(size_x,0));
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

        Reflect(CheckBoxList)
        (
            (std::vector<std::string>)(labels),
            (std::optional<std::vector<int>>)(initial_selection),
            (std::optional<bool>)(packed),
        )

        struct State
        {
            bool state = 0;
        };

        std::vector<State> current_state; // Can't use a vector of `bool` directly, as `ImGui::Checkbox` expects actual `bool`s, ugh!
        int size_x = 0; // If `packed == true`, this is set to -1 in `Init()`, and then to the column width on the first `Display()` call. Otherwise it stays at 0.

        void Init() override
        {
            if (labels.size() == 0)
                Program::Error("A checkbox list must contain at least one checkbox.");

            current_state.resize(labels.size());

            if (initial_selection)
            {
                for (int x : *initial_selection)
                {
                    if (x < 1 || x > int(labels.size()))
                        Program::Error("Index of a checkbox to be selected by default is out of range.");
                    current_state[x-1].state = true;
                }
            }

            // We can't calculate a proper button width here, as fonts don't seem to be loaded this early.
            if (packed && *packed)
                size_x = -1;
        }

        void Display(int index) override
        {
            if (size_x == -1)
            {
                for (const std::string &str : labels)
                    clamp_var_min(size_x, ImGui::CalcTextSize(str.c_str()).x);

                size_x += ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.x + ImGui::GetStyle().ItemInnerSpacing.x;
            }

            int columns = size_x ? clamp_min(int(ImGui::GetWindowContentRegionWidth()) / size_x, 1) : 1;
            int elems_per_column = columns != 1 ? (labels.size() + columns - 1) / columns : labels.size();
            columns = (labels.size() + elems_per_column - 1) / elems_per_column;

            ImGui::Columns(columns, 0, 0);

            int cur_y = ImGui::GetCursorPosY();

            int elem_index = 0;
            for (int i = 0; i < columns; i++)
            {
                for (int j = 0; j < elems_per_column; j++)
                {
                    if (elem_index >= int(labels.size()))
                        break; // I don't think we want to terminate the outer loop early. We want to use all columns no matter what.

                    ImGui::Checkbox(Str(Data::EscapeStringForWidgetName(labels[elem_index]), "###", index, ":", elem_index).c_str(), &current_state[elem_index].state);
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

        Reflect(RadioButtonList)
        (
            (std::vector<std::string>)(labels),
            (std::optional<int>)(initial_selection),
            (std::optional<bool>)(packed),
        )

        int current_selection = 0;
        int size_x = 0; // If `packed == true`, this is set to -1 in `Init()`, and then to the column width on the first `Display()` call. Otherwise it stays at 0.

        void Init() override
        {
            if (labels.size() == 0)
                Program::Error("A checkbox list must contain at least one checkbox.");

            if (initial_selection)
            {
                current_selection = *initial_selection;
                if (current_selection < 1 || current_selection > int(labels.size()))
                    Program::Error("Index of a radio button to be selected by default is out of range.");
            }

            // We can't calculate a proper button width here, as fonts don't seem to be loaded this early.
            if (packed && *packed)
                size_x = -1;
        }

        void Display(int index) override
        {
            if (size_x == -1)
            {
                for (const std::string &str : labels)
                    clamp_var_min(size_x, ImGui::CalcTextSize(str.c_str()).x);

                size_x += ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.x + ImGui::GetStyle().ItemInnerSpacing.x;
            }

            int columns = size_x ? clamp_min(int(ImGui::GetWindowContentRegionWidth()) / size_x, 1) : 1;
            int elems_per_column = columns != 1 ? (labels.size() + columns - 1) / columns : labels.size();
            columns = (labels.size() + elems_per_column - 1) / elems_per_column;

            ImGui::Columns(columns, 0, 0);

            int cur_y = ImGui::GetCursorPosY();

            int elem_index = 0;
            for (int i = 0; i < columns; i++)
            {
                for (int j = 0; j < elems_per_column; j++)
                {
                    if (elem_index >= int(labels.size()))
                        break; // I don't think we want to terminate the outer loop early. We want to use all columns no matter what.

                    ImGui::RadioButton(Str(Data::EscapeStringForWidgetName(labels[elem_index]), "###", index, ":", elem_index).c_str(), &current_selection, elem_index+1);
                    elem_index++;
                }

                ImGui::NextColumn();
                if (i != columns-1)
                    ImGui::SetCursorPosY(cur_y);
            }
        }
    };
}

struct State
{
    bool exit_requested = 0;

    virtual void Tick() = 0;
    virtual ~State() = default;
};

struct StateMain : State
{
    bool first_tick = 1;
    Data::Procedure proc;
    std::size_t current_step_index = 0;
    bool first_tick_at_this_step = 1;

    StateMain()
    {
        Load("test_procedure.json");
    }

    void Load(std::string file_name)
    {
        Json json(MemoryFile(file_name).string(), 64);

        try
        {
            proc = Data::ReflectedObjectFromJson<Data::Procedure>(json);

            if (proc.name.find_first_of("\n") != std::string::npos)
                Program::Error("Line feeds are not allowed in procedure names.");
            if (proc.steps.size() < 1)
                Program::Error("The procedure must have at least one step.");

            for (const Data::ProcedureStep &step : proc.steps)
            {
                if (step.name.find_first_of("\n") != std::string::npos)
                    Program::Error("Line feeds are not allowed in procedure names.");
            }
        }
        catch (std::exception &e)
        {
            Program::Error("While loading `", file_name, "`: ", e.what());
        }
    }

    void Tick() override
    {
        Data::ProcedureStep &current_step = proc.steps[current_step_index];

        constexpr int window_flags =
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_HorizontalScrollbar;

        ImGui::SetNextWindowPos(ivec2(VisualOptions::outer_margin));
        ImGui::SetNextWindowSize(window.Size() - ivec2(VisualOptions::outer_margin * 2));

        bool keep_running = 1;
        ImGui::Begin((Data::EscapeStringForWidgetName(proc.name) + "###procedure").c_str(), &keep_running, window_flags);
        if (!keep_running)
            exit_requested = 1;

        ImGui::Columns(2, "main_columns", 1);

        if (first_tick)
        {
            int list_column_width = 0;

            for (const Data::ProcedureStep &step : proc.steps)
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

        ImGui::EndChild();

        bool finishing_step_at_this_tick = 0;

        { // Ending step
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
            if (exit_requested)
            {
                exit_requested = 0;

                if (proc.confirm_exit && *proc.confirm_exit)
                    ImGui::OpenPopup("confirm_exit_modal");
                else
                    Program::Exit();
            }

            if (ImGui::BeginPopupModal("confirm_exit_modal", 0, VisualOptions::modal_window_flags))
            {
                ImGui::TextUnformatted("Прервать действие и выйти?");
                if (ImGui::Button("Выйти"))
                {
                    Program::Exit();
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

        window.SwapBuffers();
    }
}
