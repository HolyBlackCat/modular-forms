#pragma once

#include <memory>
#include <string>
#include <utility>

#include <SDL2/SDL_loadso.h>

#include "macros/finally.h"
#include "program/errors.h"
#include "strings/common.h"


class SharedLibrary
{
    struct Data
    {
        std::string file_name;
        void *handle = 0;

        Data(std::string file_name) : file_name(file_name)
        {
            handle = SDL_LoadObject(file_name.c_str());
            if (!handle)
                Program::Error("Unable to load a shared library: `", file_name, "`.\nReason: `", Strings::Trim(SDL_GetError()), "`.");
            // Not needed because nothing can throw below this point.
            // FINALLY_ON_THROW( SDL_UnloadObject(data.handle); )
        }

        Data(const Data &) = delete;
        Data &operator=(const Data &) = delete;

        ~Data()
        {
            SDL_UnloadObject(handle);
        }
    };

    std::shared_ptr<Data> data;

  public:
    SharedLibrary() {}

    // SDL must be initialized for these constructors to work.
    // The loaded library is ref-counted.
    SharedLibrary(std::string file_name)
        : data(std::make_shared<Data>(std::move(file_name))) {}
    SharedLibrary(const char *file_name)
        : data(std::make_shared<Data>(file_name)) {}

    [[nodiscard]] explicit operator bool() const
    {
        return bool(data);
    }

    [[nodiscard]] std::string FileName() const // Empty string if the object is null.
    {
        if (!*this)
            return "";
        return data->file_name;
    }

    [[nodiscard]] void *Handle() const
    {
        return data->handle;
    }

    [[nodiscard]] void *GetFunctionOrNull(const std::string &name) const
    {
        return GetFunctionOrNull(name.c_str());
    }
    [[nodiscard]] void *GetFunctionOrNull(const char *name) const
    {
        if (!*this)
            return 0;
        return SDL_LoadFunction(data->handle, name);
    }

    [[nodiscard]] void *GetFunction(const std::string &name) const // Throws on failure.
    {
        return GetFunction(name.c_str());
    }
    [[nodiscard]] void *GetFunction(const char *name) const // Throws on failure.
    {
        if (!*this)
            Program::Error("Attempt to load a function from a null shared library.");
        void *ret = GetFunctionOrNull(name);
        if (!ret)
            Program::Error("No function `", name, "` in shared library `", FileName(), "`.");
        return ret;
    }
};
