#pragma once

#include <map>
#include <string>
#include <vector>
#include <optional>

#include "reflection/complete.h"

#include "main/widgets.h"

namespace Data
{
    ReflectStruct(ProcedureStep,(
        (std::string)(name),
        (std::optional<bool>)(confirm),
        (std::vector<Widgets::Widget>)(widgets),
    ))

    struct Procedure
    {
        Reflect(Procedure)
        (
            (std::string)(name),
            (int)(current_step)(=0),
            (std::optional<bool>)(confirm_exit),
            (std::vector<ProcedureStep>)(steps),
        )

        std::string base_dir;
    };
}
