#pragma once

#include <map>
#include <string>
#include <vector>
#include <optional>

#include "reflection/full_with_poly.h"
#include "reflection/short_macros.h"
#include "utils/shared_library.h"

#include "main/images.h"
#include "main/widgets.h"

namespace Data
{
    SIMPLE_STRUCT( ProcedureStep
        DECL(std::string) name
        DECL(bool INIT=false ATTR Refl::Optional) confirm
        DECL(std::vector<Widgets::Widget>) widgets
    )

    // Should return 0 on success or a error message on failure.
    // Caller shouldn't free the string.
    // The error string has to remain allocated at least until the next library call.
    using external_func_ptr_t = const char *(*)();

    struct LibraryFunc
    {
        MEMBERS(
            DECL(std::string) id
            DECL(std::string) name
        )

        external_func_ptr_t ptr = 0;
    };


    struct Library
    {
        MEMBERS(
            DECL(std::string) id
            DECL(std::string) file
            DECL(std::vector<LibraryFunc>) functions
        )

        SharedLibrary library;
    };

    struct Procedure
    {
        MEMBERS(
            DECL(std::string) name
            DECL(int INIT=-1) current_step
            DECL(bool INIT=false ATTR Refl::Optional) confirm_exit
            DECL(std::vector<Library> ATTR Refl::Optional) libraries
            DECL(std::vector<ProcedureStep>) steps
        )

        fs::path resource_dir;
        mutable Image::Cache image_cache;

        bool IsTemplate() const
        {
            return current_step == -1;
        }
    };
}
