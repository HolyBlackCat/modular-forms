#pragma once

#include <memory>
#include <set>
#include <string>

#include "graphics/texture.h"
#include "program/errors.h"
#include "utils/memory_file.h"

namespace Data // Images
{
    class Image
    {
      public:
        std::string file_name;
        Graphics::TexObject texture;
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
                texture = Graphics::TexObject(nullptr);
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
    inline const Image *clicked_image = 0;
}
