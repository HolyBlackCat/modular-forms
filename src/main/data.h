#pragma once

#include <map>
#include <set>
#include <string>
#include <type_traits>
#include <vector>

#include <imgui.h>

#include "graphics/texture.h"
#include "program/errors.h"
#include "reflection/complete.h"
#include "utils/poly_storage.h"
#include "utils/json.h"
#include "utils/memory_file.h"
#include "utils/strings.h"

namespace Data // Strings
{
    inline const std::string zero_width_space = "\xEF\xBB\xBF";

    // `##` has a special meaning for ImGui when used in widget names, so we break those with zero-width spaces.
    inline std::string EscapeStringForWidgetName(const std::string &source_str)
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
}

namespace Data // Widget system and JSON serialization
{
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
    template <typename T> void ReflectedObjectToJsonLow(const T &object, int indent_steps, int indent_step_w, std::string &output);

    struct WidgetInternalFunctions
    {
        using Widget = Poly::Storage<WidgetBase_Low, WidgetInternalFunctions>;

        bool is_reflected = 0;
        const char *internal_name = 0;
        void (*from_json)(Poly::Param<WidgetBase_Low>, Json::View, std::string);
        void (*to_json)(Poly::Param<const WidgetBase_Low>, int, int, std::string &);

        template <typename T> constexpr void _make()
        {
            is_reflected = Refl::is_reflected<T>;
            internal_name = T::internal_name;
            from_json = [](Poly::Param<WidgetBase_Low> obj, Json::View view, std::string elem_name)
            {
                if constexpr (Refl::is_reflected<T>)
                    obj.template get<T>() = ReflectedObjectFromJson<T>(view[elem_name]);
            };
            to_json = [](Poly::Param<const WidgetBase_Low> obj, int indent_steps, int indent_step_w, std::string &str)
            {
                if constexpr (Refl::is_reflected<T>)
                {
                    ReflectedObjectToJsonLow(obj.template get<T>(), indent_steps, indent_step_w, str);
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
        else if constexpr (Refl::is_reflected<T>)
        {
            using refl_t = Refl::Interface<T>;
            auto refl = Refl::Interface(object);

            std::string indent_step_str(indent_step_w, ' ');
            std::string cur_indent_str(indent_step_w * indent_steps, ' ');

            if constexpr (refl_t::is_structure)
            {
                bool multiline = 0;
                refl.for_each_field([&](auto index)
                {
                    constexpr int i = index.value;
                    using field_type = typename refl_t::template field_type<i>;
                    if constexpr (!is_std_optional<field_type>::value)
                    {
                        if (!Refl::Interface<field_type>::is_primitive)
                            multiline = 1;
                    }
                    else
                    {
                        if (!Refl::Interface<typename field_type::value_type>::is_primitive)
                            multiline = 1;
                    }
                });

                output += '{';
                if (multiline)
                    output += '\n';

                bool first = 1;
                refl.for_each_field([&](auto index)
                {
                    constexpr int i = index.value;
                    using field_type = typename refl_t::template field_type<i>;
                    const std::string &field_name = refl.field_name(i);
                    auto &field_ref = refl.template field_value<i>();

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
            else if constexpr (refl_t::is_container)
            {
                if (refl.empty())
                {
                    output += "[]";
                    return;
                }

                output += "[\n";

                refl.for_each_element([&](auto it)
                {
                    output += cur_indent_str + indent_step_str;
                    ReflectedObjectToJsonLow(*it, indent_steps+1, indent_step_w, output);
                    output += ",\n";
                });

                output += cur_indent_str + "]";
            }
            else
            {
                static_assert(Meta::value<false, T>, "This type is not supported.");
            }
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
            MemoryFile::Save(file_name, ptr, ptr + str.size());
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

namespace Data // Images
{
    class Image
    {
      public:
        std::string file_name;
        Graphics::TexObject texture = nullptr;
        ivec2 pixel_size = ivec2(0);

      private:
        struct LoadedImage
        {
            std::shared_ptr<Image> data;

            operator const std::string &() const
            {
                DebugAssert("Loaded image handle is null.", bool(data));
                return data->file_name;
            }

            friend bool operator<(const std::string &a, const std::string &b)
            {
                return a < b;
            }
        };

        inline static std::set<LoadedImage, std::less<>> loaded_images;

      public:
        Image(std::string file_name) : file_name(file_name) // Don't use this constructor directly, as it doesn't provide caching. Use `Load()` instead.
        {
            MemoryFile file(file_name);
            try
            {
                Graphics::Image image(file);
                pixel_size = image.Size();
                texture = Graphics::TexObject();
                Graphics::TexUnit(texture).Interpolation(Graphics::linear).Wrap(Graphics::fill).SetData(image);
            }
            catch (std::exception &e)
            {
                Program::Error("While loading image `", file_name, "`: ", e.what());
            }
        }

        static std::shared_ptr<Image> Load(std::string file_name)
        {
            auto it = loaded_images.find(file_name);
            if (it == loaded_images.end())
            {
                return loaded_images.emplace(LoadedImage{std::make_shared<Image>(file_name)}).first->data;
            }
            else
            {
                return it->data;
            }
        }

        Image(const Image &) = delete;
        Image &operator=(const Image &) = delete;

        ~Image()
        {
            auto it = loaded_images.find(file_name);
            DebugAssert("Unable to destroy Image: not in the cache.", it != loaded_images.end());
            loaded_images.erase(it);
        }

        ImTextureID TextureHandle() const
        {
            return (ImTextureID)(uintptr_t)texture.Handle();
        }
    };

    [[maybe_unused]] // Clang is silly.
    inline const Image *last_clicked_image = 0;
}
