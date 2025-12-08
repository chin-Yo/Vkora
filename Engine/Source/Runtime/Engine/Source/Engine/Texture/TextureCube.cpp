#include "Engine/Texture/TextureCube.hpp"

#include <ktx.h>
#include <ktxvulkan.h>

#include "Misc/FileLoader.hpp"


TextureCube::TextureCube(const std::string& name, std::vector<uint8_t>&& data, std::vector<Mipmap>&& mipmaps)
    : Texture(name, VK_IMAGE_VIEW_TYPE_CUBE, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, std::move(data), std::move(mipmaps))
{
    set_layers(6);
}
