#pragma once

#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>

inline std::string GenerateGUID()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);

    std::stringstream ss;
    ss << std::hex;
    for (int i = 0; i < 8; ++i) ss << dis(gen);
    ss << "-";
    for (int i = 0; i < 4; ++i) ss << dis(gen);
    ss << "-4"; // UUID v4
    for (int i = 0; i < 3; ++i) ss << dis(gen);
    ss << "-";
    ss << dis(gen) % 4 + 8; // Variant
    for (int i = 0; i < 3; ++i) ss << dis(gen);
    ss << "-";
    for (int i = 0; i < 12; ++i) ss << dis(gen);
    return ss.str();
}

inline std::string CalculateFileHash(const std::filesystem::path& filepath)
{
    std::ifstream file(filepath, std::ios::binary);
    if (!file) return "";

    auto fileSize = std::filesystem::file_size(filepath);
    auto modTime = std::filesystem::last_write_time(filepath).time_since_epoch().count();
    return std::to_string(fileSize) + "-" + std::to_string(modTime);
}


class AssetImporter
{
public:
    void ScanAndImport(const std::string& assetRootPath);

private:
    void ImportNewAsset(const std::filesystem::path& relativeAssetPath);
    void CheckForModification(const std::filesystem::path& assetPath, const std::filesystem::path& metaPath);

    std::filesystem::path m_assetRootPath;
};
