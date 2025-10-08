#pragma once

#include <string>

class Paths
{
public:
    /**
     * Get the complete path of the current executable file
     * @return Returns a string containing the path of the current executable file
     * @note On Windows platform, the path is obtained using GetModuleFileNameA
     *       On Linux platform, the path is obtained by reading the /proc/self/exe symbolic link
     *       On macOS platform, the path is obtained using _NSGetExecutablePath
     *       On other platforms, a std::runtime_error exception will be thrown */
    static std::string GetCurrentExecutablePath();

    static std::string GetAssetPath();

    static std::string GetAssetFullPath(const std::string &relativePath);

    static std::string GetShaderPath();

    static std::string GetShaderFullPath(const std::string& relativePath);
};
