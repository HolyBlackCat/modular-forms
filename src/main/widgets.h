#pragma once

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

#include "program/errors.h"
#include "reflection/full_with_poly.h"
#include "stream/save_to_file.h"
#include "strings/common.h"
#include "utils/json.h"
#include "utils/poly_storage.h"
#include "utils/unicode.h"

namespace Data
{
    struct Procedure;
}

namespace Widgets
{
    struct WidgetBase_Low
    {
        WidgetBase_Low() = default;
        WidgetBase_Low(const WidgetBase_Low &) = default;
        WidgetBase_Low(WidgetBase_Low &&) = default;
        WidgetBase_Low &operator=(const WidgetBase_Low &) = default;
        WidgetBase_Low &operator=(WidgetBase_Low &&) = default;

        virtual void Init(const Data::Procedure &) {}
        virtual void Display(int index, bool allow_modification) = 0;
        virtual ~WidgetBase_Low() = default;
    };

    template <typename T> T ReflectedObjectFromJson(Json::View view);
    template <typename T> void ReflectedObjectToJsonLow(const T &object, int indent_steps, int indent_step_w, std::string &output);

    struct WidgetInternalFunctions
    {
        using Widget = Poly::Storage<WidgetBase_Low, WidgetInternalFunctions>;

        bool is_reflected = 0;
        const char *internal_name = 0;
        void (*from_json)(Widget &, Json::View, std::string);
        void (*to_json)(const Widget &, int, int, std::string &);

        template <typename T> constexpr void _make()
        {
            is_reflected = Refl::Class::members_known<T>;
            internal_name = T::internal_name;
            from_json = [](Widget &obj, Json::View view, std::string elem_name)
            {
                if constexpr (Refl::Class::members_known<T>)
                    obj.template derived<T>() = ReflectedObjectFromJson<T>(view[elem_name]);
            };
            to_json = [](const Widget &obj, int indent_steps, int indent_step_w, std::string &str)
            {
                if constexpr (Refl::Class::members_known<T>)
                {
                    ReflectedObjectToJsonLow(obj.template derived<T>(), indent_steps, indent_step_w, str);
                }
                else
                {
                    (void)obj;
                    (void)indent_steps;
                    (void)indent_step_w;
                    (void)str;
                }
            };
        }
    };
    using Widget = WidgetInternalFunctions::Widget;

    struct WidgetTypeManager
    {
        template <typename T> friend class WidgetBase;

        std::map<std::string, Widget(*)()> factory_funcs;

        template <typename T> void Register(std::string name)
        {
            factory_funcs.insert({name, []{return Widget::make<T>();}});
        }

      public:
        WidgetTypeManager() {}

        Widget MakeWidget(std::string name) const
        {
            auto it = factory_funcs.find(name);
            if (it == factory_funcs.end())
                Program::Error("Invalid widget name: `", name, "`.");
            return it->second();
        }
    };
    inline WidgetTypeManager &widget_type_manager()
    {
        static WidgetTypeManager ret;
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
        else if constexpr (std::is_same_v<T, fs::path>)
        {
            return ReflectedObjectFromJson<std::string>(view);
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
            T ret = widget_type_manager().MakeWidget(view["type"].GetString());
            ret.dynamic().from_json(ret, view, "data");
            return ret;
        }
        else
        {
            T ret;

            if constexpr (Refl::Class::members_known<T>)
            {
                Meta::cexpr_for<Refl::Class::member_count<T>>([&](auto index)
                {
                    constexpr int i = index.value;
                    using field_type = Refl::Class::member_type<T, i>;
                    std::string field_name = Refl::Class::MemberName<T>(i);
                    auto &field_ref = Refl::Class::Member<i>(ret);
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
            else
            {
                if constexpr (Refl::impl::StdContainer::is_container<T>)
                {
                    int elements = view.GetArraySize();

                    for (int i = 0; i < elements; i++)
                    {
                        if constexpr (Meta::is_detected<Refl::impl::StdContainer::has_single_arg_insert, T>)
                            ret.insert(ReflectedObjectFromJson<typename T::value_type>(view[i]));
                        else
                            ret.push_back(ReflectedObjectFromJson<typename T::value_type>(view[i]));
                    }
                }
                else
                {
                    static_assert(Meta::value<false, T>, "This type is not supported.");
                }
            }

            return ret;
        }
    }

    template <typename T> void InitializeWidgetsRecursively(T &object, const Data::Procedure &proc)
    {
        if constexpr (std::is_same_v<T, Widget>)
        {
            object->Init(proc);
        }
        else if constexpr (Refl::Class::members_known<T>)
        {
            Meta::cexpr_for<Refl::Class::member_count<T>>([&](auto index)
            {
                constexpr int i = index.value;
                using field_type = Refl::Class::member_type<T,i>;
                auto &field_ref = Refl::Class::Member<i>(object);
                if constexpr (!is_std_optional<field_type>::value)
                {
                    InitializeWidgetsRecursively(field_ref, proc);
                }
                else
                {
                    if (field_ref)
                        InitializeWidgetsRecursively(*field_ref, proc);
                }
            });
        }
        else if constexpr (Refl::impl::StdContainer::is_container<T>)
        {
            for (auto &it : object)
                InitializeWidgetsRecursively(it, proc);
        }
    }

    // `output` is `void output(const std::string &str)`, it's called repeadetly to compose the JSON.
    template <typename T> void ReflectedObjectToJsonLow(const T &object, int indent_steps, int indent_step_w, std::string &output)
    {
        if constexpr (std::is_same_v<T, bool>)
        {
            output += (object ? "true" : "false");
        }
        else if constexpr (std::is_same_v<T, std::string>)
        {
            output += '"';
            for (uint32_t ch : Unicode::Iterator(object))
            {
                if (ch < ' ')
                {
                    switch (ch)
                    {
                      case '\\':
                        output += R"(\\)";
                        break;
                      case '"':
                        output += R"(\")";
                        break;
                      case '\b':
                        output += R"(\b)";
                        break;
                      case '\f':
                        output += R"(\f)";
                        break;
                      case '\n':
                        output += R"(\n)";
                        break;
                      case '\r':
                        output += R"(\r)";
                        break;
                      case '\t':
                        output += R"(\t)";
                        break;
                      default:
                        output += Str(R"(\u)", std::setfill('0'), std::setw(4), std::hex, ch);
                        break;
                    }
                }
                else
                {
                    if (ch <= 0xffff)
                    {
                        if (ch < 128)
                        {
                            output += char(ch);
                        }
                        else if (ch < 2048) // 2048 = 2^11
                        {
                            output += char(0b1100'0000 + (ch >> 6));
                            output += char(0b1000'0000 + (ch & 0b0011'1111));
                        }
                        else
                        {
                            output += char(0b1110'0000 + (ch >> 12));
                            output += char(0b1000'0000 + ((ch >> 6) & 0b0011'1111));
                            output += char(0b1000'0000 + (ch & 0b0011'1111));
                        }
                    }
                }
            }
            output += '"';
        }
        else if constexpr (std::is_same_v<T, fs::path>)
        {
            ReflectedObjectToJsonLow(object.string(), indent_steps, indent_step_w, output);
        }
        else if constexpr (std::is_integral_v<T>)
        {
            output += std::to_string(object);
        }
        else if constexpr (std::is_floating_point_v<T>)
        {
            output += Str(std::setprecision(1000000), object); // There ain't such thing as too much precision.
        }
        else if constexpr (std::is_same_v<T, Widget>)
        {
            std::string indent_step_str(indent_step_w, ' ');
            std::string cur_indent_str(indent_step_w * indent_steps, ' ');

            output += "{\n";
            output += cur_indent_str + indent_step_str + R"("type" : ")" + object.dynamic().internal_name + R"(",)" + '\n';
            if (object.dynamic().is_reflected)
            {
                output += cur_indent_str + indent_step_str + R"("data" : )";
                object.dynamic().to_json(object, indent_steps+1, indent_step_w, output);
                output += ",\n";
            }

            output += cur_indent_str + "}";
        }
        else if constexpr (Refl::Class::members_known<T>)
        {
            std::string indent_step_str(indent_step_w, ' ');
            std::string cur_indent_str(indent_step_w * indent_steps, ' ');

            bool multiline = 0;
            Meta::cexpr_for<Refl::Class::member_count<T>>([&](auto index)
            {
                constexpr int i = index.value;
                using field_type = Refl::Class::member_type<T,i>;
                if constexpr (!is_std_optional<field_type>::value)
                {
                    if (Refl::Class::members_known<field_type> || Refl::impl::StdContainer::is_container<field_type>)
                        multiline = 1;
                }
                else
                {
                    if (Refl::Class::members_known<typename field_type::value_type> || Refl::impl::StdContainer::is_container<typename field_type::value_type>)
                        multiline = 1;
                }
            });

            output += '{';
            if (multiline)
                output += '\n';

            bool first = 1;
            Meta::cexpr_for<Refl::Class::member_count<T>>([&](auto index)
            {
                constexpr int i = index.value;
                using field_type = Refl::Class::member_type<T,i>;
                const std::string &field_name = Refl::Class::MemberName<T>(i);
                auto &field_ref = Refl::Class::Member<i>(object);

                if constexpr (is_std_optional<field_type>::value)
                    if (!field_ref)
                        return;

                if (first)
                {
                    first = 0;
                }
                else
                {
                    output += ',';
                    if (multiline)
                        output += '\n';
                    else
                        output += ' ';
                }

                if (multiline)
                    output += cur_indent_str + indent_step_str;
                output += R"(")" + field_name + R"(" : )";

                if constexpr (!is_std_optional<field_type>::value)
                    ReflectedObjectToJsonLow(field_ref, indent_steps+1, indent_step_w, output);
                else
                    ReflectedObjectToJsonLow(*field_ref, indent_steps+1, indent_step_w, output);
            });

            if (multiline)
            {
                output += ",\n";
                output += cur_indent_str;
            }
            output += '}';
        }
        else if constexpr (Refl::impl::StdContainer::is_container<T>)
        {
            std::string indent_step_str(indent_step_w, ' ');
            std::string cur_indent_str(indent_step_w * indent_steps, ' ');

            if (object.empty())
            {
                output += "[]";
                return;
            }

            output += "[\n";

            for (const auto &it : object)
            {
                output += cur_indent_str + indent_step_str;
                ReflectedObjectToJsonLow(it, indent_steps+1, indent_step_w, output);
                output += ",\n";
            }

            output += cur_indent_str + "]";
        }
        else
        {
            static_assert(Meta::value<false, T>, "This type is not supported.");
        }
    }

    template <typename T> std::string ReflectedObjectToJson(const T &object)
    {
        std::string ret;
        ReflectedObjectToJsonLow(object, 0, 4, ret);
        ret += '\n';
        return ret;
    }

    template <typename T> bool ReflectedObjectToJsonFile(const T &object, std::string file_name) // Returns 1 on success.
    {
        std::string str = ReflectedObjectToJson(object);
        const uint8_t *ptr = (const uint8_t*)str.data();
        try
        {
            Stream::SaveFile(file_name, ptr, ptr + str.size());
            return 1;
        }
        catch (...)
        {
            return 0;
        }
    }

    // Some trickery to register widgets.
    template <typename T> struct WidgetBase_impl_0 : public WidgetBase_Low
    {
        [[maybe_unused]] inline static int dummy = []{
            widget_type_manager().Register<T>(T::internal_name);
            return 0;
        }();
    };
    template <typename T, int * = &WidgetBase_impl_0<T>::dummy> struct WidgetBase_impl_1 : WidgetBase_impl_0<T> {}; // Yes, this is truly necessary.
    template <typename T> class WidgetBase : public WidgetBase_impl_1<T> {};
}
