#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <chrono>


enum class EFileOrderType
{
    Name, Time, Size
};

class Files
{
public:
    // File information extraction
    static std::string GetExtension(const std::string& path);
    static std::string GetBasename(const std::string& path);
    static std::string GetFilename(const std::string& path);
    static std::string GetDirectory(const std::string& path);
    static std::string GetModifiedTime(const std::string& path);

    static std::vector<std::string> Traverse(const std::string& path, bool is_recursive = false,
                                             EFileOrderType file_order_type = EFileOrderType::Name,
                                             bool is_reverse = false);
    /**
    * @brief Verify and clean the file base name, replacing invalid characters with underscores
    * @param basename The string of the file base name to be verified
    * @return The cleaned valid file base name, where invalid characters have been replaced with underscores *
    This function is used to handle illegal characters in the file base name, ensuring that the file name is valid in common file systems.
    In the Windows system, the following characters are not allowed: /, :, *, ?, ", <, >, |
    */
    static std::string ValidateBasename(const std::string& basename);

    static bool Exists(const std::string& path);
    static bool IsFile(const std::string& path);
    static bool IsDir(const std::string& path);
    static bool IsEmptyDir(const std::string& path);

    static bool CreateFileImpl(const std::string& filename, std::ios_base::openmode mode = std::ios_base::out);
    static bool CreateDir(const std::string& path, bool is_recursive = false);
    static bool RemoveFile(const std::string& filename);
    static bool RemoveDir(const std::string& path, bool is_recursive = false);
    static void CopyFile(const std::string& from, const std::string& to);
    static void RenameFile(const std::string& dir, const std::string& old_name, const std::string& new_name);

    static bool WriteString(const std::string& filename, const std::string& str);
    static bool LoadString(const std::string& filename, std::string& str);

    template <typename T, typename... Ts>
    static std::string Combine(const T& first, const Ts&... rest)
    {
        std::filesystem::path p(first);
        p /= Combine(rest...);
        return p.generic_string();
    }

    template <typename T>
    static T Combine(const T& t)
    {
        return t;
    }

    template <typename... Args>
    static std::string Format(const std::string& format, Args... args)
    {
        int size_s = std::snprintf(nullptr, 0, format.c_str(), args...) + 1;
        if (size_s <= 0) { throw std::runtime_error("Error during formatting."); }
        auto size = static_cast<size_t>(size_s);
        std::vector<char> buf(size);
        std::snprintf(buf.data(), size, format.c_str(), args...);
        return std::string(buf.data(), buf.data() + size - 1);
    }

    // Internal auxiliary structure, used for Traverse sorting optimization
    struct FileMetaInfo
    {
        std::string path;
        std::filesystem::file_time_type time;
        uintmax_t size;
    };
};
