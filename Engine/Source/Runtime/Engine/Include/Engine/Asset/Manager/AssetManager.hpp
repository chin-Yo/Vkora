#pragma once
#include <memory>
#include <string>
#include <unordered_map>


namespace scene
{
    struct MeshData;
}

namespace vkb
{
    class VulkanDevice;
}

class AssetManager
{
public:
    AssetManager(vkb::VulkanDevice& device);
    std::shared_ptr<scene::MeshData> GetMesh(const std::string& relativePath);

private:
    vkb::VulkanDevice& device;
    // or std::weak_ptr
    std::unordered_map<std::string, std::shared_ptr<scene::MeshData>> meshCache;
};
