#pragma once

#include <map>
#include <string>
#include <vector>
#include <optional>

#include "reflection/complete.h"
#include "utils/shared_library.h"

#include "main/widgets.h"

namespace Data
{
    ReflectStruct(ProcedureStep, (
        (std::string)(name),
        (std::optional<bool>)(confirm),
        (std::vector<Widgets::Widget>)(widgets),
    ))

    // Should return 0 on success or a error message on failure.
    // Caller shouldn't free the string.
    // The error string has to remain allocated at least until the next library call.
    using external_func_ptr_t = const char *(*)();

    struct LibraryFunc
    {
        Reflect(LibraryFunc)
        (
            (std::string)(id),
            (std::string)(name),
            (std::optional<bool>)(optional),
        )

        external_func_ptr_t ptr = 0;
    };


    struct Library
    {
        Reflect(Library)
        (
            (std::string)(id),
            (std::string)(file),
            (std::vector<LibraryFunc>)(functions),
        )

        SharedLibrary library;
    };

    struct Procedure
    {
        Reflect(Procedure)
        (
            (std::string)(name),
            (int)(current_step)(=0),
            (std::optional<bool>)(confirm_exit),
            (std::optional<std::vector<Library>>)(libraries),
            (std::vector<ProcedureStep>)(steps),
        )

        std::string base_dir;
    };
}
