#include "Misc/FileLoader.hpp"
#include <fstream>
#include <filesystem>

std::vector<uint32_t> FileLoader::ReadShaderBinaryU32(const std::string &filename)
{
    std::vector<uint8_t> binaryData = ReadFileBinary(filename);

    // Check if the data size is a multiple of 4 (since we need to convert it to uint32_t)
    if (binaryData.size() % sizeof(uint32_t) != 0)
    {
        throw std::runtime_error("Shader binary file size is not a multiple of 4 bytes");
    }

    std::vector<uint32_t> result;
    result.resize(binaryData.size() / sizeof(uint32_t));
    std::memcpy(result.data(), binaryData.data(), binaryData.size());

    return result;
}

std::vector<uint8_t> FileLoader::ReadFileBinary(const std::filesystem::path &path)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file: " + path.string());
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
    if (!file.read(reinterpret_cast<char *>(buffer.data()), size))
    {
        throw std::runtime_error("Failed to read file: " + path.string());
    }

    return buffer;
}

std::string FileLoader::ReadFileString(const std::filesystem::path &path)
{
    std::vector<uint8_t> binaryData = ReadFileBinary(path);
    return std::string(reinterpret_cast<const char *>(binaryData.data()), binaryData.size());
}

std::string FileLoader::ReadTextFile(const std::string &filename)
{
    return FileLoader::ReadFileString(filename);
}