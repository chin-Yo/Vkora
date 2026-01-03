#pragma once

#include "Engine/Texture/Texture.hpp"


namespace vkb
{
    class Sampler;
}

class TextureCube : public Texture
{
    friend class TextureFactory;

public:
    TextureCube(const std::string& name, std::vector<uint8_t>&& data = {}, std::vector<Mipmap>&& mipmaps = {{}});

    std::weak_ptr<vkb::Sampler> sampler;

    void CreateMipmapViews();

protected:
    std::unordered_set<std::unique_ptr<vkb::ImageView>> MipmapViews;
};
