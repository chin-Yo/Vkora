#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vulkan/vulkan_core.h>


namespace vkb
{
    class Sampler;
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

namespace vkb
{
    class VulkanDevice;
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

    std::shared_ptr<Texture2D> GetTexture(const std::string& relativePath = "Textures/demo1_22.png");
    void LodaAllTexture();


    const std::unordered_map<std::string, std::shared_ptr<Texture2D>>& GetTextureCache() const;

    const std::unordered_map<std::string, std::shared_ptr<scene::MeshData>>& GetMeshCache() const;

private:
    vkb::VulkanDevice& device;
    std::shared_ptr<vkb::Sampler> defaultSampler = nullptr;

    std::vector<VkDescriptorSet> textures_id;
    // or std::weak_ptr
    std::unordered_map<std::string, std::shared_ptr<scene::MeshData>> meshCache;

    std::unordered_map<std::string, std::shared_ptr<Texture2D>> textureCache;

    //std::unordered_map<SamplerKey, std::shared_ptr<vkb::Sampler>> samplerCache;
};
