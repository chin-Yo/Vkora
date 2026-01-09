#include "Engine/Asset/Manager/AssetManager.hpp"

#include <ktx.h>
#include <ktxvulkan.h>
#include <stb_image.h>
#include <tiny_gltf.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "GlobalContext.hpp"
#include "backends/imgui_impl_vulkan.h"
#include "Engine/Asset/AssetRegistry.hpp"
#include "Engine/Asset/Import/ModelLoader.hpp"
#include "Engine/SceneGraph/ComponentPool.hpp"
#include "Engine/SceneGraph/Node.hpp"
#include "Engine/SceneGraph/Components/Material.hpp"
#include "Engine/SceneGraph/Components/SubMesh.hpp"
#include "Engine/Texture/Texture2D.hpp"
#include "Engine/Texture/TextureFactory.hpp"
#include "Framework/Core/ImageView.hpp"
#include "Logging/Logger.hpp"
#include "Misc/Paths.hpp"
#include "VkPreset/VpSampler.hpp"
#include "Framework/Core/Sampler.hpp"
#include "Misc/FileLoader.hpp"
#include "Misc/Files.hpp"
#include "World/WorldManager.hpp"

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

void AssetManager::ConstructDefaultTexture()
{
    std::vector<uint8_t> data(24, 0);
    auto texture_cube = TextureFactory::CreateTextureCubeFromMemory(
        DEFAULT_IrradianceMap, std::move(data), 1, 1,
        VK_FORMAT_R8G8B8A8_UNORM);
    texture_cube->CopyDataToGPU(device);
    texture_cube->sampler = cubeSampler;
    textureCubeCache[DEFAULT_IrradianceMap] = std::move(texture_cube);
    Texture* Tex_vilidity = GetTextureCube(DEFAULT_IrradianceMap);
    assert(Tex_vilidity != nullptr && "DEFAULT_IrradianceMap is null"); //---

    std::vector<uint8_t> NorData{128, 128, 255, 255};
    auto texture_normal = TextureFactory::CreateTexture2DFromMemory(DEFAULT_NormalMap, std::move(NorData),
                                                                    1, 1, VK_FORMAT_R8G8B8A8_UNORM);
    texture_normal->CopyDataToGPU(device);
    texture_normal->sampler = defaultSampler;
    texture_normal->texture_id = ImGui_ImplVulkan_AddTexture(texture_normal->sampler.lock()->GetHandle(),
                                                             texture_normal->get_vk_image_view().GetHandle(),
                                                             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    textures_id.push_back(texture_normal->texture_id);
    texture2DCache[DEFAULT_NormalMap] = std::move(texture_normal);
    Tex_vilidity = GetTexture(DEFAULT_NormalMap);
    assert(Tex_vilidity != nullptr && "DEFAULT_NormalMap is null"); //---

    auto texture_PrefilterMap = TextureFactory::CreateTextureCubeFromMemory(DEFAULT_PrefilterMap, {}, 512, 512,
                                                                            VK_FORMAT_R16G16B16A16_SFLOAT,
                                                                            std::vector<Texture::Mipmap>(8));
    texture_PrefilterMap->create_vk_image(device);
    texture_PrefilterMap->TransitionImageLayout(device, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    texture_PrefilterMap->sampler = cubeSampler;
    textureCubeCache[DEFAULT_PrefilterMap] = std::move(texture_PrefilterMap);
    Texture* PrefilterMap = GetTextureCube(DEFAULT_PrefilterMap);
    assert(PrefilterMap != nullptr && "DEFAULT_PrefilterMap is null"); //---

    Texture* BRDF = GetTexture(DEFAULT_BRDFLUT);
    assert(BRDF != nullptr && "DEFAULT_BRDFLUT is null");
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

Texture2D* AssetManager::GetTexture(const std::string& relativePath)
{
    auto it = texture2DCache.find(relativePath);
    if (it != texture2DCache.end())
    {
        return it->second.get();
    }
    return nullptr;
}

TextureCube* AssetManager::GetTextureCube(const std::string& relativePath)
{
    auto it = textureCubeCache.find(relativePath);
    if (it != textureCubeCache.end())
    {
        return it->second.get();
    }
    return nullptr;
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
            VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
            auto newTexture = TextureFactory::CreateTexture2DFromMemory(texture->relativePath, raw_data,
                                                                        (uint32_t)width, height,
                                                                        req_comp, format);
            newTexture->CopyDataToGPU(device);
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
                    format);
                newTexture->CopyDataToGPU(device);
                newTexture->sampler = cubeSampler;
                textureCubeCache[texture->relativePath] = std::move(newTexture);
            }
            else if (ktxtex->numLayers == 1)
            {
                auto newTexture = TextureFactory::CreateTexture2DFromMemory(
                    texture->relativePath, ktx_data, ktx_size, width, height
                    , format);
                newTexture->CopyDataToGPU(device);
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
    ConstructDefaultTexture();
}

void AssetManager::LoadGlTF(const std::string& relativePath)
{
    auto Dir = Files::GetDirectory(relativePath) + "/";
    tinygltf::Model model;
    ModelLoader::GetInstance().LoadGltfModel(model, Paths::GetAssetFullPath(relativePath));
    auto& glTfScene = model.scenes[model.defaultScene >= 0 ? model.defaultScene : 0];
    auto* ActiveScene = GRuntimeGlobalContext.worldManager->GetActiveWorld();
    std::unique_ptr<scene::Node> SceneNodeUni = std::make_unique<scene::Node>(
        ActiveScene, !glTfScene.name.empty() ? glTfScene.name : "Scene");
    auto* SceneProxyNode = SceneNodeUni.get();
    ActiveScene->AddNode(std::move(SceneNodeUni));
    // 递归函数：输入当前节点索引 + 父世界 TRS，输出所有带 mesh 节点的世界 TRS
    std::vector<std::tuple<int, glm::vec3, glm::quat, glm::vec3>> meshInstances; // meshIdx, worldT, worldR, worldS

    std::function<void(int, const glm::vec3&, const glm::quat&, const glm::vec3&)> traverse =
        [&](int nodeIdx, const glm::vec3& parentWorldT, const glm::quat& parentWorldR, const glm::vec3& parentWorldS)
    {
        const auto& gltfNode = model.nodes[nodeIdx];

        // ❌ 如果节点用了 matrix，跳过（你不用 matrix）
        if (!gltfNode.matrix.empty())
        {
            // 可选：警告，但继续遍历子节点（用父 TRS 作为当前世界）
            if (gltfNode.mesh >= 0)
            {
                meshInstances.emplace_back(gltfNode.mesh, parentWorldT, parentWorldR, parentWorldS);
            }
            for (int child : gltfNode.children)
            {
                traverse(child, parentWorldT, parentWorldR, parentWorldS);
            }
            return;
        }

        // 1. 获取局部 TRS（带默认值）
        glm::vec3 localT = gltfNode.translation.empty()
                               ? glm::vec3(0.0f)
                               : glm::make_vec3(gltfNode.translation.data());

        glm::quat localR = gltfNode.rotation.empty()
                               ? glm::quat(1.0f, 0.0f, 0.0f, 0.0f)
                               : glm::quat(
                                   static_cast<float>(gltfNode.rotation[3]), // w
                                   static_cast<float>(gltfNode.rotation[0]), // x
                                   static_cast<float>(gltfNode.rotation[1]), // y
                                   static_cast<float>(gltfNode.rotation[2]) // z
                               );

        glm::vec3 localS = gltfNode.scale.empty()
                               ? glm::vec3(1.0f)
                               : glm::make_vec3(gltfNode.scale.data());

        // 2. 合成世界 TRS（关键！）
        glm::vec3 worldT = parentWorldT + parentWorldR * (parentWorldS * localT);
        glm::quat worldR = parentWorldR * localR;
        glm::vec3 worldS = parentWorldS * localS;

        // 3. 如果有 mesh，记录
        if (gltfNode.mesh >= 0)
        {
            meshInstances.emplace_back(gltfNode.mesh, worldT, worldR, worldS);
        }

        // 4. 递归子节点
        for (int childIdx : gltfNode.children)
        {
            traverse(childIdx, worldT, worldR, worldS);
        }
    };

    for (int rootNodeIdx : glTfScene.nodes)
    {
        traverse(rootNodeIdx, glm::vec3(0), glm::quat(1, 0, 0, 0), glm::vec3(1));
    }

    //为每个 primitive 创建 node，并直接设置 TRS（无矩阵！）
    for (const auto& [meshIdx, worldT, worldR, worldS] : meshInstances)
    {
        const auto& mesh = model.meshes[meshIdx];
        for (size_t primIdx = 0; primIdx < mesh.primitives.size(); ++primIdx)
        {
            auto* node = SceneProxyNode->CreateChild(
                "Mesh_" + std::to_string(meshIdx) + "_Prim_" + std::to_string(primIdx)
            );

            //直接设置 TRS，完全不用矩阵
            node->GetTransform().SetTranslation(worldT);
            node->GetTransform().SetRotation(worldR);
            node->GetTransform().SetScale(worldS);

            auto* subMeshCom = ActiveScene->GetComponentManager()->AddComponent<::scene::SubMesh>(node);

            auto OptionalMeshData = ModelLoader::LoadPrimitiveAsMesh(model, mesh.primitives[primIdx]);
            if (OptionalMeshData.has_value())
            {
                auto newMesh = std::make_shared<scene::MeshData>();
                auto success = ModelLoader::MeshToBuffer(
                    device,
                    *newMesh, *OptionalMeshData);
                if (success)
                {
                    subMeshCom->SetMeshData(newMesh);
                }
            }
            if (mesh.primitives[primIdx].material >= 0)
            {
                int matIdx = mesh.primitives[primIdx].material;
                MaterialTexturePaths texPaths = ModelLoader::ExtractMaterialTexturePaths(model, matIdx);
                subMeshCom->get_mut_material()->base_color_texture = GetTexture(Dir + texPaths.baseColor);
                subMeshCom->get_mut_material()->normal_texture = GetTexture(Dir + texPaths.normal);
            }
        }
    }
}

const std::unordered_map<std::string, std::unique_ptr<Texture2D>>& AssetManager::GetTexture2DCache() const
{
    return texture2DCache;
}

const std::unordered_map<std::string, std::unique_ptr<TextureCube>>& AssetManager::GetTextureCubeCache() const
{
    return textureCubeCache;
}

const std::unordered_map<std::string, std::shared_ptr<scene::MeshData>>& AssetManager::GetMeshCache() const
{
    return meshCache;
}
