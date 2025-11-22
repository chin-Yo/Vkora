#include "Engine/Asset/Manager/AssetManager.hpp"

#include <stb_image.h>

#include "backends/imgui_impl_vulkan.h"
#include "Engine/Asset/AssetRegistry.hpp"
#include "Engine/Asset/Import/ModelLoader.hpp"
#include "Engine/SceneGraph/Components/SubMesh.hpp"
#include "Engine/Texture/Texture2D.hpp"
#include "Framework/Core/ImageView.hpp"
#include "Logging/Logger.hpp"
#include "Misc/Paths.hpp"
#include "VkPreset/VpSampler.hpp"
#include "Framework/Core/Sampler.hpp"

AssetManager::~AssetManager()
{
    for (auto it : textures_id)
    {
        ImGui_ImplVulkan_RemoveTexture(it);
    }
}

AssetManager::AssetManager(vkb::VulkanDevice& device)
    : device(device)
{
}

std::shared_ptr<scene::MeshData> AssetManager::GetMesh(const std::string& relativePath)
{
    auto it = meshCache.find(relativePath);
    if (it != meshCache.end())
    {
        return it->second;
    }
    auto newMesh = std::make_shared<scene::MeshData>();
    auto mesh = ModelLoader::GetInstance().LoadAsSingleMesh(Paths::GetAssetFullPath(relativePath));
    if (mesh.has_value())
    {
        auto success = ModelLoader::MeshToBuffer(
            device,
            *newMesh, *mesh);
        if (!success)
        {
            LOG_ERROR("Failed to load mesh")
            return nullptr;
        }
    }
    meshCache[relativePath] = std::move(newMesh);
    return meshCache[relativePath];
}

std::shared_ptr<Texture2D> AssetManager::GetTexture(const std::string& relativePath)
{
    auto it = textureCache.find(relativePath);
    if (it != textureCache.end())
    {
        return it->second;
    }
    auto newTexture = std::make_shared<Texture2D>(Paths::ExtractBasename(relativePath),
                                                  Paths::GetAssetFullPath(relativePath),
                                                  Texture::ContentType::Color);
    newTexture->MoveToGPU(device);

    if (!defaultSampler)
    {
        defaultSampler = std::make_shared<vkb::Sampler>(device, vp::DefaultSamplerPreset{}.CreateInfo());
    }
    newTexture->sampler = defaultSampler;


    newTexture->texture_id = ImGui_ImplVulkan_AddTexture(newTexture->sampler.lock()->GetHandle(),
                                                         newTexture->get_vk_image_view().GetHandle(),
                                                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    textures_id.push_back(newTexture->texture_id);
    textureCache[relativePath] = std::move(newTexture);
    return textureCache[relativePath];
}

void AssetManager::LodaAllTexture()
{
    auto textures = AssetRegistry::Get().GetAllAssetsOfType(AssetType::Texture);
    for (auto& texture : textures)
    {
        GetTexture(texture->relativePath);
    }
}

const std::unordered_map<std::string, std::shared_ptr<Texture2D>>& AssetManager::GetTextureCache() const
{
    return textureCache;
}

const std::unordered_map<std::string, std::shared_ptr<scene::MeshData>>& AssetManager::GetMeshCache() const
{
    return meshCache;
}
