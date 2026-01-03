#include "Engine/Texture/TextureCube.hpp"

#include <ktx.h>
#include <ktxvulkan.h>

#include "Misc/FileLoader.hpp"


TextureCube::TextureCube(const std::string& name, std::vector<uint8_t>&& data, std::vector<Mipmap>&& mipmaps)
    : Texture(name, VK_IMAGE_VIEW_TYPE_CUBE, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, std::move(data), std::move(mipmaps))
{
    set_layers(6);
}

void TextureCube::CreateMipmapViews()
{
    for (uint32_t i = 0; i < mipmaps.size(); ++i)
    {
        auto VkView = std::make_unique<vkb::ImageView>(*vk_image, VK_IMAGE_VIEW_TYPE_CUBE, format, i, 0, 1, 6);
        vk_image->add_view(VkView.get());
        MipmapViews.insert(std::move(VkView));
    }
}
