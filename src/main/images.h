#pragma once

#include <memory>
#include <set>
#include <string>

#include "imgui.h"

#include "graphics/texture.h"
#include "program/errors.h"
#include "stream/readonly_data.h"

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

      public:
        class Cache
        {
            using image_set_t = std::set<LoadedImage, std::less<>>;
            std::shared_ptr<image_set_t> loaded_images;

          public:
            Cache() : loaded_images(std::make_shared<image_set_t>()) {}
            Cache(const Cache &other) : loaded_images(other.loaded_images) {}
            Cache &operator=(const Cache &other) {loaded_images = other.loaded_images; return *this;}

            [[nodiscard]] std::shared_ptr<Image> Load(std::string file_name)
            {
                auto it = loaded_images->find(file_name);
                if (it == loaded_images->end())
                    return loaded_images->emplace(LoadedImage{std::make_shared<Image>(file_name)}).first->data;
                else
                    return it->data;
            }

            void Reset()
            {
                loaded_images->clear();
            }
        };

        // Don't use this constructor directly, as it doesn't do caching. Use `Cache::Load()` instead.
        Image(std::string file_name) : file_name(file_name)
        {
            Stream::ReadOnlyData file(file_name);
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

        ImTextureID TextureHandle() const
        {
            return (ImTextureID)(uintptr_t)texture.Handle();
        }
    };

    [[maybe_unused]] // Clang is silly.
    inline const Image *clicked_image = 0;
}
