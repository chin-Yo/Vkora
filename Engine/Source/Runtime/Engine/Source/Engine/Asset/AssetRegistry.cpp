#include "Engine/Asset/AssetRegistry.hpp"

#include <fstream>
#include <iostream>
#include <shared_mutex>
#include <nlohmann/json.hpp>

#include "Engine/Asset/Meta/Mesh.hpp"
#include "Logging/Logger.hpp"

using json = nlohmann::json;

AssetRegistry& AssetRegistry::Get()
{
    static AssetRegistry instance;
    return instance;
}

void AssetRegistry::ScanDirectory(const std::string& assetRootPath)
{
    AssetRootPath = assetRootPath;
    AssetMetas.clear();

    if (!std::filesystem::exists(AssetRootPath))
    {
        LOG_ERROR("[AssetRegistry] Error: Asset root path does not exist: " + AssetRootPath)
        return;
    }

    LOG_INFO("[AssetRegistry] Scanning for .meta files in: {}", AssetRootPath)
    for (const auto& entry : std::filesystem::recursive_directory_iterator(AssetRootPath))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".meta")
        {
            LoadMetadataFromFile(entry.path());
        }
    }
    LOG_INFO("[AssetRegistry] Registered {} assets.", std::to_string(AssetMetas.size()))
}

const AssetMetadata* AssetRegistry::GetByGUID(const std::string& guid) const
{
    auto it = PersistentIdToRuntimeId.find(guid);
    if (it != PersistentIdToRuntimeId.end())
    {
        return AssetMetas.at(it->second).get();
    }
    return nullptr;
}

std::vector<const AssetMetadata*> AssetRegistry::GetAllAssetsOfType(AssetType type) const
{
    std::shared_lock lock(Mutex);

    std::vector<const AssetMetadata*> result;
    auto it = TypeToIdsIndex.find(type);
    if (it != TypeToIdsIndex.end())
    {
        const auto& ids = it->second;
        result.reserve(ids.size());
        for (AssetID id : ids)
        {
            result.push_back(AssetMetas.at(id).get());
        }
    }
    return result;
}

std::filesystem::path AssetRegistry::GetFullPath(const AssetMetadata& metadata)
{
    return std::filesystem::path(AssetRootPath) / metadata.relativePath;
}

std::filesystem::path AssetRegistry::GetFullPath(const std::string& guid)
{
    const AssetMetadata* meta = GetByGUID(guid);
    if (meta)
    {
        return GetFullPath(*meta);
    }
    return {};
}

bool AssetRegistry::LoadMetadataFromFile(const std::filesystem::path& metaFilePath)
{
    std::ifstream f(metaFilePath);
    if (!f.is_open())
    {
        LOG_ERROR("AssetRegistry", "Error: Failed to open meta file: " + metaFilePath.string())
        return false;
    }

    try
    {
        json metaJson = json::parse(f);

        std::string guid = metaJson.at("guid");
        if (PersistentIdToRuntimeId.count(guid))
        {
            LOG_ERROR("AssetRegistry", "Error: Duplicate GUID '{}' found in file {}. Skipping.", guid,
                      metaFilePath.string())
            return false;
        }

        AssetType type = StringToAssetType(metaJson.at("type"));
        if (type == AssetType::Unknown)
        {
            return false;
        }

        std::unique_ptr<AssetMetadata> metadata = std::make_unique<AssetMetadata>();
        AssetID newId = NextRuntimeID++;

        switch (type)
        {
        case AssetType::Mesh:
            metadata = std::make_unique<MeshAssetMetaData>();
            break;
        case AssetType::Texture:
            metadata = std::make_unique<TextureAssetMetaData>();
            break;
        default:
            metadata = std::make_unique<AssetMetadata>();
            break;
        }

        metadata->guid = guid;
        metadata->type = type;

        TypeToIdsIndex[metadata->type].push_back(newId);

        std::filesystem::path assetRelativePath(metaJson.at("asset_path").get<std::string>());
        metadata->relativePath = assetRelativePath.generic_string();
        PathToIdIndex[metadata->relativePath] = newId;

        metadata->name = assetRelativePath.stem().string();

        if (metaJson.contains("source_file_hash"))
        {
            metadata->sourceFileHash = metaJson.at("source_file_hash");
        }
        AssetMetas[newId] = std::move(metadata);
    }
    catch (const json::exception& e)
    {
        LOG_ERROR("[AssetRegistry] Error: Failed to parse meta file: {}. Details:{}", metaFilePath.string(), e.what())
        return false;
    }

    return true;
}

AssetType AssetRegistry::StringToAssetType(const std::string& typeStr)
{
    if (typeStr == "Mesh") return AssetType::Mesh;
    if (typeStr == "Texture") return AssetType::Texture;
    if (typeStr == "Material") return AssetType::Material;
    if (typeStr == "Shader") return AssetType::Shader;
    if (typeStr == "Scene") return AssetType::Scene;
    return AssetType::Unknown;
}

std::string AssetRegistry::AssetTypeToString(AssetType type)
{
    switch (type)
    {
    case AssetType::Mesh: return "Mesh";
    case AssetType::Texture: return "Texture";
    case AssetType::Material: return "Material";
    case AssetType::Shader: return "Shader";
    case AssetType::Scene: return "Scene";
    default: return "Unknown";
    }
}

std::string AssetRegistry::GetAssetTypeStringFromExtension(const std::string& ext)
{
    if (ext == ".fbx" || ext == ".obj" || ext == ".gltf") return "Mesh";
    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp") return "Texture";
    return "Unknown";
}
