#include "Engine/Texture/TextureFactory.hpp"


std::unique_ptr<Texture2D> TextureFactory::CreateTexture2DFromMemory(
    const std::string& name, std::vector<uint8_t>&& data, uint32_t width,
    uint32_t height, Texture::ContentType content_type)
{
    auto tex = std::make_unique<Texture2D>(name, std::move(data));
    // _SRGB = automatically handles gamma correction; used for color display.
    // _UNORM = raw linear data; used for computation and non-color purposes.
    tex->set_format(content_type == Texture::Color ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM);
    tex->set_height(height);
    tex->set_width(width);
    tex->set_depth(1u);
    return tex;
}

std::unique_ptr<Texture2D> TextureFactory::CreateTexture2DFromMemory(
    const std::string& name, const uint8_t* data, uint32_t width, uint32_t height, uint32_t req_comp,
    Texture::ContentType content_type)
{
    auto tex = std::make_unique<Texture2D>(name);
    tex->set_data(data, width * height * req_comp);
    tex->set_format(content_type == Texture::Color ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM);
    tex->set_height(height);
    tex->set_width(width);
    tex->set_depth(1u);
    return tex;
}

std::unique_ptr<Texture2D> TextureFactory::CreateTexture2DFromMemory(const std::string& name, const uint8_t* data,
                                                                     size_t size, uint32_t width, uint32_t height,
                                                                     Texture::ContentType content_type)
{
    auto tex = std::make_unique<Texture2D>(name);
    tex->set_data(data, size);
    tex->set_format(content_type == Texture::Color ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM);
    tex->set_height(height);
    tex->set_width(width);
    tex->set_depth(1u);
    return tex;
}

std::unique_ptr<TextureCube> TextureFactory::CreateTextureCubeFromMemory(const std::string& name,
                                                                         std::vector<uint8_t>&& data, uint32_t width,
                                                                         uint32_t height,
                                                                         Texture::ContentType content_type)
{
    auto tex = std::make_unique<TextureCube>(name, std::move(data));
    tex->set_format(content_type == Texture::Color ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM);
    tex->set_height(height);
    tex->set_width(width);
    tex->set_depth(1u);
    return tex;
}

std::unique_ptr<TextureCube> TextureFactory::CreateTextureCubeFromMemory(const std::string& name, const uint8_t* data,
                                                                         size_t size, uint32_t width, uint32_t height,
                                                                         Texture::ContentType content_type)
{
    auto tex = std::make_unique<TextureCube>(name);
    tex->set_data(data, size);
    tex->set_width(width);
    tex->set_height(height);
    tex->set_format(content_type == Texture::Color ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM);
    tex->set_depth(1u);
    return tex;
}
