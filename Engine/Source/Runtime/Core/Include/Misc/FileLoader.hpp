#pragma once

#include <string>
#include <vector>
#include <filesystem>

class FileLoader
{
public:
    static std::vector<uint32_t> FileLoader::ReadShaderBinaryU32(const std::string &filename);

    static std::vector<uint8_t> FileLoader::ReadFileBinary(const std::filesystem::path &path);

    static std::string ReadFileString(const std::filesystem::path &path);

    static std::string ReadTextFile(const std::string &filename);
};