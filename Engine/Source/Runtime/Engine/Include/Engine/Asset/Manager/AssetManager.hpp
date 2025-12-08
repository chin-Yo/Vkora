#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vulkan/vulkan_core.h>


class Texture;
class TextureCube;

namespace vkb
{
    class Sampler;
    class VulkanDevice;
}

class Texture2D;

namespace scene
{
    class Sampler;
}

namespace scene
{
    struct MeshData;
}

class AssetManager
{
private:
    struct SamplerKey
    {
        VkFilter magFilter;
        VkFilter minFilter;
        VkSamplerAddressMode addressModeU;
        VkSamplerAddressMode addressModeV;
        float maxAnisotropy;

        bool operator==(const SamplerKey& other) const
        {
            return magFilter == other.magFilter &&
                minFilter == other.minFilter &&
                addressModeU == other.addressModeU &&
                addressModeV == other.addressModeV &&
                maxAnisotropy == other.maxAnisotropy;
        }
    };

public:
    AssetManager() = delete;
    ~AssetManager();
    AssetManager(vkb::VulkanDevice& device);
    std::shared_ptr<scene::MeshData> GetMesh(const std::string& relativePath);

    template <class TextureClass>
    std::unique_ptr<TextureClass> GetTexture(const std::string& relativePath = "Textures/Default.png");

    void LodaAllTexture();


    const std::unordered_map<std::string, std::unique_ptr<Texture2D>>& GetTexture2DCache() const;

    const std::unordered_map<std::string, std::shared_ptr<scene::MeshData>>& GetMeshCache() const;

private:
    vkb::VulkanDevice& device;
    std::shared_ptr<vkb::Sampler> defaultSampler = nullptr;
    std::shared_ptr<vkb::Sampler> cubeSampler = nullptr;


    std::vector<VkDescriptorSet> textures_id;
    // or std::weak_ptr
    std::unordered_map<std::string, std::shared_ptr<scene::MeshData>> meshCache;

    std::unordered_map<std::string, std::unique_ptr<Texture2D>> texture2DCache;

    std::unordered_map<std::string, std::unique_ptr<TextureCube>> textureCubeCache;

    //std::unordered_map<SamplerKey, std::shared_ptr<vkb::Sampler>> samplerCache;
};

template <class TextureClass>
std::unique_ptr<TextureClass> AssetManager::GetTexture(const std::string& relativePath)
{
    if constexpr (std::is_same_v<TextureClass, Texture2D>)
    {
        auto it = texture2DCache.find(relativePath);
        if (it != texture2DCache.end())
        {
            return std::move(it->second);
        }
    }
    else if constexpr (std::is_same_v<TextureClass, TextureCube>)
    {
        auto it = textureCubeCache.find(relativePath);
        if (it != textureCubeCache.end())
        {
            return std::move(it->second);
        }
    }
    else
    {
        static_assert(sizeof(TextureClass) == 0, "Unsupported texture type in AssetManager::GetTexture");
    }
    return nullptr;
}
