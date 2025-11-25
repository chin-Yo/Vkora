#include "Misc/Files.hpp"

#include <fstream>

#include "Logging/Logger.hpp"


std::string Files::GetExtension(const std::string& path)
{
    std::string extension = std::filesystem::path(path).extension().generic_string();
    if (!extension.empty() && extension[0] == '.')
    {
        extension.erase(0, 1);
    }
    return extension;
}

std::string Files::GetBasename(const std::string& path)
{
    return std::filesystem::path(path).stem().generic_string();
}

std::string Files::GetFilename(const std::string& path)
{
    return std::filesystem::path(path).filename().generic_string();
}

std::string Files::GetDirectory(const std::string& path)
{
    return std::filesystem::path(path).parent_path().generic_string();
}

std::string Files::GetModifiedTime(const std::string& path)
{
    std::error_code ec;
    if (IsFile(path))
    {
        auto ftime = std::filesystem::last_write_time(path, ec);
        if (ec) return "0";
        return std::to_string(std::chrono::duration_cast<std::chrono::seconds>(ftime.time_since_epoch()).count());
    }

    if (IsDir(path))
    {
        long long max_time = 0;
        // 注意：这里调用 traverse 可能会递归，需注意性能。
        // 如果仅需顶层文件，这里用 false。保持原逻辑用 true。
        std::vector<std::string> files = Traverse(path, true);
        for (const std::string& file : files)
        {
            if (IsFile(file))
            {
                auto ftime = std::filesystem::last_write_time(file, ec);
                if (!ec)
                {
                    auto count = std::chrono::duration_cast<std::chrono::seconds>(ftime.time_since_epoch()).count();
                    max_time = std::max(max_time, count);
                }
            }
        }
        return std::to_string(max_time);
    }
    return "";
}

std::vector<std::string> Files::Traverse(const std::string& path, bool is_recursive,
                                         EFileOrderType file_order_type, bool is_reverse)
{
    std::vector<std::string> filenames;
    std::error_code ec;

    if (!std::filesystem::exists(path, ec))
    {
        return filenames;
    }

    // Cache the metadata to avoid performance issues during sorting.
    std::vector<FileMetaInfo> metas;

    auto process_entry = [&](const std::filesystem::directory_entry& entry)
    {
        FileMetaInfo meta;
        meta.path = entry.path().generic_string();

        // Only obtain the time and size when necessary, as this may involve system calls.
        if (file_order_type == EFileOrderType::Time)
        {
            meta.time = entry.last_write_time(ec);
        }
        else if (file_order_type == EFileOrderType::Size)
        {
            // The folder does not have a size, to prevent anomalies.
            meta.size = entry.is_regular_file(ec) ? entry.file_size(ec) : 0;
        }
        metas.push_back(meta);
    };

    if (is_recursive)
    {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(path, ec))
        {
            process_entry(entry);
        }
    }
    else
    {
        for (const auto& entry : std::filesystem::directory_iterator(path, ec))
        {
            process_entry(entry);
        }
    }

    std::sort(metas.begin(), metas.end(),
              [file_order_type, is_reverse](const FileMetaInfo& lhs, const FileMetaInfo& rhs)
              {
                  bool result = false;
                  switch (file_order_type)
                  {
                  case EFileOrderType::Name:
                      result = lhs.path < rhs.path;
                      break;
                  case EFileOrderType::Time:
                      result = lhs.time < rhs.time;
                      break;
                  case EFileOrderType::Size:
                      result = lhs.size < rhs.size;
                      break;
                  }
                  return is_reverse ? !result : result;
              });
    filenames.reserve(metas.size());
    for (const auto& meta : metas)
    {
        filenames.push_back(meta.path);
    }

    return filenames;
}

std::string Files::ValidateBasename(const std::string& basename)
{
    static const std::string invalid_chars = "/:*?\"<>|";

    std::string validated_basename = basename;
    for (char& c : validated_basename)
    {
        if (invalid_chars.find(c) != std::string::npos)
        {
            c = '_';
        }
    }
    return validated_basename;
}

bool Files::Exists(const std::string& path)
{
    std::error_code ec;
    return std::filesystem::exists(path, ec);
}

bool Files::IsFile(const std::string& path)
{
    std::error_code ec;
    return std::filesystem::is_regular_file(path, ec);
}

bool Files::IsDir(const std::string& path)
{
    std::error_code ec;
    return std::filesystem::is_directory(path, ec);
}

bool Files::IsEmptyDir(const std::string& path)
{
    std::error_code ec;
    if (!std::filesystem::is_directory(path, ec)) return false;
    return std::filesystem::is_empty(path, ec);
}

bool Files::CreateFile(const std::string& filename, std::ios_base::openmode mode)
{
    if (Exists(filename))
    {
        return false;
    }

    std::filesystem::path p(filename);
    if (p.has_parent_path())
    {
        std::error_code ec;
        std::filesystem::create_directories(p.parent_path(), ec);
    }

    std::ofstream ofs(filename, mode);
    bool success = ofs.is_open();
    ofs.close();
    return success;
}

bool Files::CreateDir(const std::string& path, bool is_recursive)
{
    if (Exists(path))
    {
        return false;
    }

    std::error_code ec;
    if (is_recursive)
    {
        return std::filesystem::create_directories(path, ec);
    }
    return std::filesystem::create_directory(path, ec);
}

bool Files::RemoveFile(const std::string& filename)
{
    std::error_code ec;
    if (!std::filesystem::exists(filename, ec))
    {
        return false;
    }
    return std::filesystem::remove(filename, ec);
}

bool Files::RemoveDir(const std::string& path, bool is_recursive)
{
    std::error_code ec;
    if (!std::filesystem::exists(path, ec))
    {
        return false;
    }

    if (is_recursive)
    {
        return std::filesystem::remove_all(path, ec) > 0;
    }
    return std::filesystem::remove(path, ec);
}

void Files::CopyFile(const std::string& from, const std::string& to)
{
    std::error_code ec;
    std::filesystem::copy(from, to, std::filesystem::copy_options::overwrite_existing, ec);
    if (ec)
    {
        LOG_WARN("Copy file failed: {} -> {}", from, to);
    }
}

void Files::RenameFile(const std::string& dir, const std::string& old_name, const std::string& new_name)
{
    if (old_name == new_name) return;

    std::filesystem::path p_dir(dir);
    std::filesystem::path p_old = p_dir / old_name;
    std::filesystem::path p_new = p_dir / new_name;

    std::error_code ec;
    std::filesystem::rename(p_old, p_new, ec);
    if (ec)
    {
        LOG_WARN("Rename file error: {}", ec.message());
    }
}

bool Files::WriteString(const std::string& filename, const std::string& str)
{
    std::ofstream file(filename);
    if (!file.is_open())
    {
        LOG_ERROR("failed to write string file {}", filename);
        return false;
    }

    file.write(str.data(), str.size());
    file.close();

    return true;
}

bool Files::LoadString(const std::string& filename, std::string& str)
{
    std::ifstream file(filename, std::ios::in | std::ios::binary);
    if (!file.is_open())
    {
        LOG_ERROR("failed to load string file {}", filename);
        return false;
    }

    file.seekg(0, std::ios::end);
    size_t size = static_cast<size_t>(file.tellg());

    if (size > 0)
    {
        str.resize(size);
        file.seekg(0, std::ios::beg);
        file.read(&str[0], size);
    }
    else
    {
        str.clear();
    }

    file.close();
    return true;
}
