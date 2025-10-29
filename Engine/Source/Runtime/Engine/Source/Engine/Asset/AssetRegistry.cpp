#include "Engine/Asset/AssetRegistry.hpp"

#include <fstream>
#include <iostream>
#include <shared_mutex>
#include <nlohmann/json.hpp>

#include "Engine/Asset/Meta/Mesh.hpp"

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
        std::cerr << "[AssetRegistry] Error: Asset root path does not exist: " << AssetRootPath << std::endl;
        return;
    }

    std::cout << "[AssetRegistry] Scanning for .meta files in: " << AssetRootPath << std::endl;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(AssetRootPath))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".meta")
        {
            LoadMetadataFromFile(entry.path());
        }
    }
    std::cout << "[AssetRegistry] Scan finished. Registered " << AssetMetas.size() << " assets." << std::endl;
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
    return {}; // 返回空路径
}

bool AssetRegistry::LoadMetadataFromFile(const std::filesystem::path& metaFilePath)
{
    std::ifstream f(metaFilePath);
    if (!f.is_open())
    {
        std::cerr << "[AssetRegistry] Error: Failed to open meta file: " << metaFilePath.string() << std::endl;
        return false;
    }

    try
    {
        json metaJson = json::parse(f);

        std::string guid = metaJson.at("guid");
        if (PersistentIdToRuntimeId.count(guid))
        {
            std::cerr << "[AssetRegistry] Warning: Duplicate GUID '" << guid << "' found in file "
                << metaFilePath.string() << ". Skipping." << std::endl;
            return false;
        }

        AssetType type = StringToAssetType(metaJson.at("type"));
        if (type == AssetType::Unknown)
        {
            return false; // 不支持的类型
        }

        std::unique_ptr<AssetMetadata> metadata = std::make_unique<AssetMetadata>();
        AssetID newId = NextRuntimeID++;

        // 根据类型创建具体的元数据对象
        switch (type)
        {
        case AssetType::Mesh:
            metadata = std::make_unique<MeshAssetMetadata>();
        // 在这里可以解析 Mesh 特有的导入设置
            break;
        case AssetType::Texture:
            //metadata = std::make_unique<TextureAssetMetadata>();
            // 在这里可以解析 Texture 特有的导入设置
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
        std::cerr << "[AssetRegistry] Error: Failed to parse meta file: " << metaFilePath.string()
            << ". Details: " << e.what() << std::endl;
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
