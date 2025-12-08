#include "VkPreset/VpSampler.hpp"

namespace vp
{
    VkSamplerCreateInfo DefaultSamplerPreset::CreateInfo() const
    {
        VkSamplerCreateInfo info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        info.pNext = nullptr;
        info.magFilter = VK_FILTER_LINEAR; // Magnification filter using linear interpolation
        info.minFilter = VK_FILTER_LINEAR; // Minification filter using linear interpolation
        info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR; // Mipmap mode using linear filtering
        info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT; // U coordinate addressing mode, repeat texture
        info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT; // V coordinate addressing mode, repeat texture
        info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT; // W coordinate addressing mode, repeat texture
        info.anisotropyEnable = VK_TRUE; // Enable anisotropic filtering
        info.maxAnisotropy = 16.0f; // Maximum anisotropy value set to 16
        info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK; // Border color set to opaque black
        info.unnormalizedCoordinates = VK_FALSE; // Use normalized texture coordinates
        info.compareEnable = VK_FALSE; // Disable comparison operation
        info.minLod = 0.0f; // Minimum LOD level set to 0
        info.maxLod = VK_LOD_CLAMP_NONE; // Maximum LOD level with no restriction
        return info;
    }


    ShadowMapSamplerPreset::ShadowMapSamplerPreset(VkCompareOp op): compareOp_(op)
    {
    }

    VkSamplerCreateInfo ShadowMapSamplerPreset::CreateInfo() const
    {
        VkSamplerCreateInfo info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        info.pNext = nullptr;
        info.magFilter = VK_FILTER_LINEAR;
        info.minFilter = VK_FILTER_LINEAR;
        info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        info.compareEnable = VK_TRUE;
        info.compareOp = compareOp_;
        info.maxLod = 0.0f;
        return info;
    }

    VkSamplerCreateInfo CubeMapSamplerPreset::CreateInfo() const
    {
        VkSamplerCreateInfo info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        info.magFilter = VK_FILTER_LINEAR;
        info.minFilter = VK_FILTER_LINEAR;
        info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        info.anisotropyEnable = VK_TRUE;
        info.maxAnisotropy = 16.0f;
        info.compareEnable = VK_FALSE;
        info.unnormalizedCoordinates = VK_FALSE;
        info.minLod = 0.0f;
        info.maxLod = VK_LOD_CLAMP_NONE;
        info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        info.pNext = nullptr;
        return info;
    }
}
