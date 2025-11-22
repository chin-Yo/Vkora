#pragma once

#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>

#include "Misc/PicoSHA2.hpp"

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
    std::error_code ec;
    if (!std::filesystem::exists(filepath, ec))
    {
        return "";
    }
    auto sz = std::filesystem::file_size(filepath, ec);
    if (ec) return "";

    auto ftime = std::filesystem::last_write_time(filepath, ec);
    if (ec) return "";

    auto sctp = std::chrono::time_point_cast<std::chrono::seconds>(ftime);
    auto dur = sctp.time_since_epoch();
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(dur).count();

    std::ostringstream oss;
    oss << sz << '-' << timestamp;
    return oss.str();
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
