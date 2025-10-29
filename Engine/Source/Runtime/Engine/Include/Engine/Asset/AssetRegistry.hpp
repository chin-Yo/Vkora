#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <memory>
#include <shared_mutex>

#include "Meta/Meta.hpp"


class AssetRegistry
{
public:
    static AssetRegistry& Get();
    AssetRegistry(const AssetRegistry&) = delete;
    AssetRegistry& operator=(const AssetRegistry&) = delete;

    void ScanDirectory(const std::string& assetRootPath);

    const AssetMetadata* GetByGUID(const std::string& guid) const;

    std::vector<const AssetMetadata*> GetAllAssetsOfType(AssetType type) const;

    std::filesystem::path GetFullPath(const AssetMetadata& metadata);
    std::filesystem::path GetFullPath(const std::string& guid);


    static AssetType StringToAssetType(const std::string& typeStr);
    static std::string AssetTypeToString(AssetType type);
    static std::string GetAssetTypeStringFromExtension(const std::string& ext);

private:
    AssetRegistry() = default;
    ~AssetRegistry() = default;

    bool LoadMetadataFromFile(const std::filesystem::path& metaFilePath);

    std::string AssetRootPath;

    std::unordered_map<AssetID, std::unique_ptr<AssetMetadata>> AssetMetas;

// Index
    std::unordered_map<GUId, AssetID> PersistentIdToRuntimeId;

    std::unordered_map<std::string, AssetID> PathToIdIndex;

    std::unordered_map<AssetType, std::vector<AssetID>> TypeToIdsIndex;
// End Index

    AssetID NextRuntimeID = 1;
    mutable std::shared_mutex Mutex;
};
