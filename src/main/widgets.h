#pragma once

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

#include "program/errors.h"
#include "reflection/full_with_poly.h"
#include "reflection/short_macros.h"
#include "stream/save_to_file.h"
#include "strings/common.h"
#include "utils/poly_storage.h"
#include "utils/unicode.h"

namespace Data
{
    struct Procedure;
}

namespace Widgets
{
    STRUCT( BasicWidget POLYMORPHIC )
    {
        MEMBERS()

        virtual void Init(const Data::Procedure &) {}
        virtual void Display(int index, bool allow_modification) = 0;
    };

    using Widget = Refl::PolyStorage<BasicWidget>;

    template <typename T> void InitializeWidgetsRecursively(T &object, const Data::Procedure &proc)
    {
        if constexpr (std::is_same_v<T, Widget>)
        {
            object->Init(proc);
        }
        else if constexpr (Meta::is_specialization_of<T, std::optional>)
        {
            InitializeWidgetsRecursively(*object, proc);
        }
        else if constexpr (Refl::Class::members_known<T>)
        {
            Meta::cexpr_for<Refl::Class::member_count<T>>([&](auto index)
            {
                constexpr int i = index.value;
                InitializeWidgetsRecursively(Refl::Class::Member<i>(object), proc);
            });
        }
        else if constexpr (requires{object.begin(); object.end();})
        {
            for (auto &it : object)
                InitializeWidgetsRecursively(it, proc);
        }
    }
}
