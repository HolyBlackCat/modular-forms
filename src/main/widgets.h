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

        virtual std::string PrettyName() const = 0;
        virtual void Init(const Data::Procedure &) {}
        virtual void Display(int index, bool allow_modification) = 0;
        virtual void DisplayEditor(Data::Procedure &, int index) = 0;

        virtual bool IsEditable() const {return true;}
    };

    using Widget = Refl::PolyStorage<BasicWidget>;

    void InitializeWidgets(Data::Procedure &proc);
}
