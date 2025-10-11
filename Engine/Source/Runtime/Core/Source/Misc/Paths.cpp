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


fs::path FindProjectRoot()
{
    static const fs::path root = []()
    {
        fs::path exePath = fs::canonical(Paths::GetCurrentExecutablePath());
        fs::path current = exePath.parent_path();

        for (int i = 0; i < 8; ++i)
        {
            if (fs::exists(current / "CMakeLists.txt"))
            {
                return current;
            }
            current = current.parent_path();
        }

        throw std::runtime_error("Could not find project root directory (CMakeLists.txt not found).");
    }();
    return root;
}


std::string Paths::GetAssetPath()
{
    static const std::string path = []()
    {
        fs::path p = FindProjectRoot() / "Assets";
        if (!fs::exists(p))
        {
            throw std::runtime_error("Asset directory not found: " + p.string());
        }
        return p.string();
    }();
    return path;
}

std::string Paths::GetAssetFullPath(const std::string& relativePath)
{
    return (fs::path(GetAssetPath()) / relativePath).string();
}

std::string Paths::GetShaderPath()
{
    static const std::string path = []()
    {
        fs::path p = FindProjectRoot() / "Engine" / "Shaders";
        if (!fs::exists(p))
        {
            throw std::runtime_error("Shaders directory not found: " + p.string());
        }
        return p.string();
    }();
    return path;
}

std::string Paths::GetShaderFullPath(const std::string& relativePath)
{
    return (fs::path(GetShaderPath()) / relativePath).string();
}

std::string Paths::GetEngineRootPath()
{
    static const std::string path = FindProjectRoot().string();
    return path;
}

std::string Paths::GetContentPath()
{
    static const std::string path = []()
    {
        fs::path p = FindProjectRoot() / "Engine" / "Content";
        return p.string();
    }();
    return path;
}
