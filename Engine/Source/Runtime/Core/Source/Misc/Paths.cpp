#include "Misc/Paths.hpp"
#if defined(_WIN32)
    #include <windows.h>  // For GetModuleFileName
#elif defined(__linux__)
    #include <unistd.h>   // For readlink
    #include <limits.h>   // For PATH_MAX
#elif defined(__APPLE__)
    #include <mach-o/dyld.h> // For _NSGetExecutablePath
    #include <limits.h>      // For PATH_MAX
#else
#error "Unsupported platform"
#endif
#include <filesystem>
#include <stdexcept>


namespace fs = std::filesystem;

std::string Paths::GetCurrentExecutablePath()
{
    char path[1024] = {0};

#if defined(_WIN32)
    // Windows
    GetModuleFileNameA(nullptr, path, sizeof(path));
#elif defined(__linux__)
    // Linux
    readlink("/proc/self/exe", path, sizeof(path) - 1);
#elif defined(__APPLE__)
    // macOS
    uint32_t size = sizeof(path);
    _NSGetExecutablePath(path, &size);
#else
    throw std::runtime_error("Unsupported platform");
#endif

    return std::string(path);
}

std::string Paths::GetAssetPath()
{
    fs::path exePath = fs::canonical(GetCurrentExecutablePath());
    fs::path binDir = exePath.parent_path();

    fs::path projectRoot = binDir;
    for (int i = 0; i < 8; ++i)
    {
        if (fs::exists(projectRoot / "CMakeLists.txt"))
        {
            break;
        }
        projectRoot = projectRoot.parent_path();
    }

    if (!fs::exists(projectRoot / "CMakeLists.txt"))
    {
        throw std::runtime_error("Could not find project root directory.");
    }

    fs::path assetPath = projectRoot / "Assets";

    if (!fs::exists(assetPath))
    {
        throw std::runtime_error("Asset directory not found: " + assetPath.string());
    }

    return assetPath.string();
}

std::string Paths::GetAssetFullPath(const std::string &relativePath)
{
    return GetAssetPath() + "/" + relativePath;
}

std::string Paths::GetShaderPath()
{
    fs::path exePath = fs::canonical(GetCurrentExecutablePath());
    fs::path binDir = exePath.parent_path();

    fs::path projectRoot = binDir;
    for (int i = 0; i < 8; ++i)
    {
        if (fs::exists(projectRoot / "CMakeLists.txt"))
        {
            break;
        }
        projectRoot = projectRoot.parent_path();
    }

    if (!fs::exists(projectRoot / "CMakeLists.txt"))
    {
        throw std::runtime_error("Could not find project root directory.");
    }

    fs::path assetPath = projectRoot / "Engine/Shaders";

    if (!fs::exists(assetPath))
    {
        throw std::runtime_error("Shaders/spv directory not found: " + assetPath.string());
    }

    return assetPath.string();
}

std::string Paths::GetShaderFullPath(const std::string& relativePath)
{
    return GetShaderPath() + "/" + relativePath;
}
