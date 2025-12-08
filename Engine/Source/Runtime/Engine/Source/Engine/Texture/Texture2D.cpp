#include "Engine/Texture/Texture2D.hpp"

#include <ktx.h>
#include <ktxvulkan.h>
#include <stb_image.h>

#include "Framework/Common/VkHelpers.hpp"
#include "Framework/Core/Buffer.hpp"
#include "Framework/Core/Image.hpp"
#include "Framework/Core/Queue.hpp"
#include "Framework/Core/Sampler.hpp"
#include "Framework/Core/VulkanDevice.hpp"
#include "Logging/Logger.hpp"
#include "Misc/FileLoader.hpp"
#include "VkPreset/VpSampler.hpp"


Texture2D::Texture2D(const std::string& name, std::vector<uint8_t>&& data, std::vector<Mipmap>&& mipmaps)
    : Texture(name, VK_IMAGE_VIEW_TYPE_2D, 0, std::move(data), std::move(mipmaps))
{
    set_layers(1);
}
