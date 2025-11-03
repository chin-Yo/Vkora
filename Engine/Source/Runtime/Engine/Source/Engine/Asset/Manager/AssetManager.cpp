#include "Engine/Asset/Manager/AssetManager.hpp"

#include "Engine/Asset/Import/ModelLoader.hpp"
#include "Engine/SceneGraph/Components/SubMesh.hpp"
#include "Logging/Logger.hpp"
#include "Misc/Paths.hpp"

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
