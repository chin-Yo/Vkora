#include "Engine/Asset/Manager/AssetManager.hpp"

#include <stb_image.h>

#include "Engine/Asset/Import/ModelLoader.hpp"
#include "Engine/SceneGraph/Components/SubMesh.hpp"
#include "Engine/SceneGraph/Components/Texture.hpp"
#include "Engine/SceneGraph/Components/Image/Stb.hpp"
#include "Logging/Logger.hpp"
#include "Misc/Paths.hpp"
#include "VkPreset/VpSampler.hpp"

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

std::shared_ptr<scene::Texture> AssetManager::GetTexture(const std::string& relativePath)
{
    auto it = textureCache.find(relativePath);
    if (it != textureCache.end())
    {
        return it->second;
    }
    auto newTexture = std::make_shared<scene::Texture>(relativePath);
    int width, height, nrChannels;
    unsigned char* data = stbi_load(Paths::GetAssetFullPath(relativePath).c_str(), &width, &height, &nrChannels, 0);
    if (data == nullptr)
    {
        LOG_ERROR("Error: {} ", stbi_failure_reason())
    }
    std::vector<uint8_t> imageData(data, data + width * height * nrChannels);
    scene::Stb* stb = new scene::Stb{relativePath, imageData, scene::Image::Color};
    vkb::Sampler sampler{device, vp::DefaultSamplerPreset{}.CreateInfo()};
    scene::Sampler* samplerObj = new scene::Sampler{"DefaultSampler", std::move(sampler)};
    newTexture->set_image(*stb);
    newTexture->set_sampler(*samplerObj);
    stbi_image_free(data);
    textureCache[relativePath] = std::move(newTexture);
    return textureCache[relativePath];
}
