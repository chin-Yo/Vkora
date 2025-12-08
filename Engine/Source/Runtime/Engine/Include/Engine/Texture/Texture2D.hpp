#pragma once

#include "Engine/Texture/Texture.hpp"


namespace vkb
{
    class Sampler;
}

class Texture2D : public Texture
{
    friend class TextureFactory;

public:
    Texture2D(const std::string& name, std::vector<uint8_t>&& data = {}, std::vector<Mipmap>&& mipmaps = {{}});
    //void MoveToGPU(vkb::VulkanDevice& device);

    std::weak_ptr<vkb::Sampler> sampler;

    VkDescriptorSet texture_id = 0;

protected:
};
