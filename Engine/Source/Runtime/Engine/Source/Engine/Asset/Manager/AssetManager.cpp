#include "Engine/Asset/Manager/AssetManager.hpp"

#include <ktx.h>
#include <ktxvulkan.h>
#include <stb_image.h>

#include "backends/imgui_impl_vulkan.h"
#include "Engine/Asset/AssetRegistry.hpp"
#include "Engine/Asset/Import/ModelLoader.hpp"
#include "Engine/SceneGraph/Components/SubMesh.hpp"
#include "Engine/Texture/Texture2D.hpp"
#include "Engine/Texture/TextureFactory.hpp"
#include "Framework/Core/ImageView.hpp"
#include "Logging/Logger.hpp"
#include "Misc/Paths.hpp"
#include "VkPreset/VpSampler.hpp"
#include "Framework/Core/Sampler.hpp"
#include "Misc/FileLoader.hpp"

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
    defaultSampler = std::make_shared<vkb::Sampler>(device, vp::DefaultSamplerPreset{}.CreateInfo());
    cubeSampler = std::make_shared<vkb::Sampler>(device, vp::CubeMapSamplerPreset{}.CreateInfo());
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

void AssetManager::LodaAllTexture()
{
    auto textures = AssetRegistry::Get().GetAllAssetsOfType(AssetType::Texture);
    for (auto& texture : textures)
    {
        std::string uri = Paths::GetAssetFullPath(texture->relativePath);
        auto data = FileLoader::ReadFileBinary(uri);

        auto fun = [](const std::string& uri) -> std::string
        {
            auto dot_pos = uri.find_last_of('.');
            if (dot_pos == std::string::npos)
            {
                LOG_WARN("Uri has no extension")
                return "";
            }

            return uri.substr(dot_pos + 1);
        };
        auto extension = fun(uri);

        if (extension == "png" || extension == "jpg")
        {
            int width;
            int height;
            int comp;
            int req_comp = 4;

            auto data_buffer = reinterpret_cast<const stbi_uc*>(data.data());
            auto data_size = static_cast<int>(data.size());

            auto raw_data = stbi_load_from_memory(data_buffer, data_size, &width, &height, &comp, req_comp);

            if (!raw_data)
            {
                LOG_ERROR("Failed to load {} {}", uri, stbi_failure_reason())
                continue;
            }
            auto newTexture = TextureFactory::CreateTexture2DFromMemory(texture->relativePath, raw_data,
                                                                        (uint32_t)width, height,
                                                                        req_comp, Texture::ContentType::Other);
            newTexture->MoveToGPU(device);
            stbi_image_free(raw_data);
            newTexture->sampler = defaultSampler;
            newTexture->texture_id = ImGui_ImplVulkan_AddTexture(newTexture->sampler.lock()->GetHandle(),
                                                                 newTexture->get_vk_image_view().GetHandle(),
                                                                 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            textures_id.push_back(newTexture->texture_id);
            texture2DCache[texture->relativePath] = std::move(newTexture);
        }
        else if (extension == "ktx")
        {
            auto data_buffer = reinterpret_cast<const ktx_uint8_t*>(data.data());
            auto data_size = static_cast<int>(data.size());
            ktxTexture* ktxtex = nullptr;
            ktxTexture_CreateFromMemory(data_buffer, data_size, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxtex);
            ktx_uint8_t* ktx_data = ktxTexture_GetData(ktxtex);
            ktx_size_t ktx_size = ktxTexture_GetLevelSize(ktxtex, 0);
            uint32_t width = ktxtex->baseWidth;
            uint32_t height = ktxtex->baseHeight;
            VkFormat format = ktxTexture_GetVkFormat(ktxtex);
            if (ktxtex->isCubemap)
            {
                auto newTexture = TextureFactory::CreateTextureCubeFromMemory(
                    texture->relativePath, ktx_data, ktx_size, width, height,
                    Texture::ContentType::Other);
                newTexture->MoveToGPU(device);
                newTexture->sampler = cubeSampler;
                textureCubeCache[texture->relativePath] = std::move(newTexture);
            }
            else if (ktxtex->numLayers == 1)
            {
                auto newTexture = TextureFactory::CreateTexture2DFromMemory(
                    texture->relativePath, ktx_data, ktx_size, width, height
                    , Texture::ContentType::Other);
                newTexture->MoveToGPU(device);
                newTexture->sampler = defaultSampler;
                newTexture->texture_id = ImGui_ImplVulkan_AddTexture(newTexture->sampler.lock()->GetHandle(),
                                                                     newTexture->get_vk_image_view().GetHandle(),
                                                                     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                textures_id.push_back(newTexture->texture_id);
                texture2DCache[texture->relativePath] = std::move(newTexture);
            }
            ktxTexture_Destroy(ktxtex);
        }
    }
}

const std::unordered_map<std::string, std::unique_ptr<Texture2D>>& AssetManager::GetTexture2DCache() const
{
    return texture2DCache;
}

const std::unordered_map<std::string, std::shared_ptr<scene::MeshData>>& AssetManager::GetMeshCache() const
{
    return meshCache;
}
