#include "Engine/Asset/AssetImporter.hpp"

#include "nlohmann/json.hpp"
#include <iostream>
#include <fstream>

#include "Engine/Asset/AssetRegistry.hpp"
#include "Misc/Paths.hpp"

void AssetImporter::ScanAndImport(const std::string& assetRootPath)
{
    std::cout << "[AssetImporter] Starting scan for new and modified assets in: " << assetRootPath << std::endl;
    m_assetRootPath = std::filesystem::absolute(assetRootPath);

    for (const auto& entry : std::filesystem::recursive_directory_iterator(m_assetRootPath))
    {
        if (entry.is_regular_file())
        {
            const auto& assetFullPath = entry.path();

            if (assetFullPath.extension() == ".meta")
            {
                continue;
            }
            auto relativePath = std::filesystem::relative(assetFullPath, m_assetRootPath);
            auto metaFullPath = Paths::GetContentPath() / relativePath;
            metaFullPath += ".meta";

            if (!std::filesystem::exists(metaFullPath))
            {
                ImportNewAsset(relativePath);
            }
            else
            {
                CheckForModification(relativePath, metaFullPath);
            }
        }
    }
    std::cout << "[AssetImporter] Scan finished." << std::endl;
}

void AssetImporter::ImportNewAsset(const std::filesystem::path& relativeAssetPath)
{
    std::cout << "[AssetImporter] Found new asset, importing: " << relativeAssetPath.string() << std::endl;
    auto assetFullPath = std::filesystem::path(Paths::GetAssetFullPath(relativeAssetPath.generic_string()));

    AssetType type = AssetRegistry::StringToAssetType(
        AssetRegistry::GetAssetTypeStringFromExtension(assetFullPath.extension().string()));

    if (type == AssetType::Unknown)
    {
        std::cout << "[AssetImporter] Skipping unsupported file type: " << assetFullPath << std::endl;
        return;
    }

    nlohmann::json metaJson;
    metaJson["guid"] = GenerateGUID();
    metaJson["asset_path"] = relativeAssetPath.generic_string();
    metaJson["type"] = AssetRegistry::AssetTypeToString(type);
    metaJson["source_file_hash"] = CalculateFileHash(Paths::GetAssetFullPath(relativeAssetPath.generic_string()));

    auto metaPath = Paths::GetContentPath() / relativeAssetPath;
    metaPath += ".meta";

    std::filesystem::create_directories(metaPath.parent_path());

    std::ofstream metaFile(metaPath);
    if (!metaFile.is_open())
    {
        std::cerr << "[AssetImporter] ERROR: Cannot create meta file: " << metaPath.string() << std::endl;
        return;
    }

    metaFile << metaJson.dump(4);
    metaFile.close();

    std::cout << "[AssetImporter] Created meta file: " << metaPath.string() << std::endl;
}

void AssetImporter::CheckForModification(const std::filesystem::path& assetPath, const std::filesystem::path& metaPath)
{
    std::ifstream f(metaPath);
    if (!f.is_open())
    {
        std::cerr << "[AssetImporter] Warning: Could not open meta file for checking: " << metaPath.string() <<
            std::endl;
        return;
    }

    try
    {
        nlohmann::json metaJson = nlohmann::json::parse(f);
        f.close(); 

        std::string oldHash = metaJson.value("source_file_hash", "");
        std::string newHash = CalculateFileHash(Paths::GetAssetFullPath(assetPath.generic_string()));

        if (oldHash != newHash)
        {
            std::cout << "[AssetImporter] Asset modified: " << assetPath.string() << std::endl;

            metaJson["source_file_hash"] = newHash;
            std::ofstream outFile(metaPath);
            outFile << metaJson.dump(4);

            std::string guid = metaJson["guid"];
            std::cout << "[AssetImporter] Triggering re-import/hot-reload for GUID: " << guid << std::endl;
        }
    }
    catch (const nlohmann::json::exception& e)
    {
        std::cerr << "[AssetImporter] Error parsing meta file " << metaPath.string() << ": " << e.what() << std::endl;
    }
}
