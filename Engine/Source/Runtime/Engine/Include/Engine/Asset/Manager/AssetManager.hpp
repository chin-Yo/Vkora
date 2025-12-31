#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vulkan/vulkan_core.h>

class Texture2D;
class TextureCube;

namespace vkb
{
    class Sampler;
    class VulkanDevice;
}

namespace scene
{
    struct MeshData;
}

#define ICON_IMAGES "Textures/Icon/Icon_Images.png"

#define DEFAULT_IrradianceMap "Default_IrradianceMap"
#define DEFAULT_NormalMap "Default_NormalMap"

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
    void ConstructDefaultTexture();
    std::shared_ptr<scene::MeshData> GetMesh(const std::string& relativePath);

    Texture2D* GetTexture(const std::string& relativePath = "Textures/Default.png");

    TextureCube* GetTextureCube(const std::string& relativePath);

    void LodaAllTexture();


    const std::unordered_map<std::string, std::unique_ptr<Texture2D>>& GetTexture2DCache() const;

    const std::unordered_map<std::string, std::unique_ptr<TextureCube>>& GetTextureCubeCache() const;

    const std::unordered_map<std::string, std::shared_ptr<scene::MeshData>>& GetMeshCache() const;
    
    std::shared_ptr<vkb::Sampler> defaultSampler = nullptr;
    std::shared_ptr<vkb::Sampler> cubeSampler = nullptr;
private:
    vkb::VulkanDevice& device;
    
    std::vector<VkDescriptorSet> textures_id;
    // or std::weak_ptr
    std::unordered_map<std::string, std::shared_ptr<scene::MeshData>> meshCache;

    std::unordered_map<std::string, std::unique_ptr<Texture2D>> texture2DCache;

    std::unordered_map<std::string, std::unique_ptr<TextureCube>> textureCubeCache;

    //std::unordered_map<SamplerKey, std::shared_ptr<vkb::Sampler>> samplerCache;
};
