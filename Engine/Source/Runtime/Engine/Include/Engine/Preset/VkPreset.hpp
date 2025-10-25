#pragma once
#include <memory>
#include <volk.h>

#include "Engine/SceneGraph/Components/Image.hpp"

namespace ps
{
    /**
     * @brief A texture wrapper that owns its image data and links it with a sampler
     */
    struct Texture
    {
        std::unique_ptr<scene::Image> image;
        VkSampler sampler;
    };

    /**
     * @brief Loads in a ktx 2D texture
     * @param file The filename of the texture to load
     * @param content_type The type of content in the image file
     */
    Texture load_texture(vkb::VulkanDevice& device, const std::string& file, scene::Image::ContentType content_type);
}
