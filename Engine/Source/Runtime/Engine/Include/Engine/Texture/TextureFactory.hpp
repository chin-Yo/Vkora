#pragma once
#include <memory>

#include "Texture2D.hpp"
#include "TextureCube.hpp"


class TextureFactory
{
public:
    //static std::unique_ptr<Texture2D> CreateTexture2D(const std::string& name, const std::string& filepath);
    //static std::unique_ptr<TextureCube> CreateTextureCube(const std::string& name, const std::string& filepath);

    static std::unique_ptr<Texture2D> CreateTexture2DFromMemory(const std::string& name,
                                                                std::vector<uint8_t>&& data, uint32_t width,
                                                                uint32_t height, VkFormat format);
    static std::unique_ptr<Texture2D> CreateTexture2DFromMemory(const std::string& name,
                                                                const uint8_t* data, uint32_t width, uint32_t height,
                                                                uint32_t req_comp, VkFormat format);
    static std::unique_ptr<Texture2D> CreateTexture2DFromMemory(const std::string& name,
                                                                const uint8_t* data, size_t size, uint32_t width,
                                                                uint32_t height, VkFormat format);
    static std::unique_ptr<TextureCube> CreateTextureCubeFromMemory(const std::string& name,
                                                                    std::vector<uint8_t>&& data, uint32_t width,
                                                                    uint32_t height, VkFormat format,
                                                                    std::vector<Texture::Mipmap>&& mipmaps = {{}});
    static std::unique_ptr<TextureCube> CreateTextureCubeFromMemory(const std::string& name,
                                                                    const uint8_t* data, size_t size, uint32_t width,
                                                                    uint32_t height,
                                                                    VkFormat format);
};
