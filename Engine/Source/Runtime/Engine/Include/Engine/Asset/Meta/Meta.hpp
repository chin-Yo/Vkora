#pragma once
#include <string>

enum class AssetType
{
    Unknown,
    Mesh,
    Texture,
    Material,
    Shader,
    Scene
};

enum class AssetState
{
    Unloaded, // 仅有Meta信息，资产数据未加载到内存
    Loading, // 正在异步加载中
    Loaded, // 资产数据已在内存中，随时可用
    Unloading, // 正在卸载
    Error, // 加载或处理时发生错误
};

using AssetID = uint64_t;
using GUId = std::string;

// 通用的资产元数据基类
struct AssetMetadata
{
    std::string guid; // 全局唯一标识符 (从 .meta 文件读取)
    std::string name; // 文件名，不含扩展名 (从 asset_path 解析)
    std::string relativePath; // 资产相对于根目录的路径 (从 asset_path 读取)
    AssetType type = AssetType::Unknown;
    std::string sourceFileHash; // 源文件内容的哈希值 (用于检测修改)

    // Runtime
    AssetState state; // 当前资产在内存中的状态
    size_t fileSize; // 文件大小（字节），可用于排序或内存预算
    uint64_t lastModifiedTime; // 文件最后修改时间戳，用于热重载检测
    virtual ~AssetMetadata() = default;
};
