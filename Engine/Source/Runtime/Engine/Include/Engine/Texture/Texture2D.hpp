#pragma once

#include "Engine/Texture/Texture.hpp"


namespace vkb
{
    class Sampler;
}

class Texture2D : public Texture
{
public:
    Texture2D(const std::string& name, const std::string& uri, ContentType content_type);

    void MoveToGPU(vkb::VulkanDevice& device);

    std::weak_ptr<vkb::Sampler> sampler;

    VkDescriptorSet texture_id = 0;

protected:
};
