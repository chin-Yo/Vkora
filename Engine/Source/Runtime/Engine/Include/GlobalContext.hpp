#pragma once

#include <memory>
#include <string>
#include "WindowSystem.hpp"
#include "Async/PriorityThreadPool.hpp"

namespace vkb
{
    class VulkanDevice;
}

class AssetManager;
class WorldManager;
class RenderSystem;
class WindowSystem;

struct EngineInitParams;

/// Manage the lifetime and creation/destruction order of all global system
class RuntimeGlobalContext
{
public:
    // create all global systems and initialize these systems
    void StartSystems(const std::string& config_file_path);
    // destroy all global systems
    void ShutdownSystems();

    vkb::VulkanDevice& GetDevice()
    {
        return *device;
    }

public:
    vkb::VulkanDevice* device;
    WindowSystem* windowSystem;
    RenderSystem* renderSystem;
    WorldManager* worldManager;
    AssetManager* assetManager;
    PriorityThreadPool pThreadPool;
};

extern RuntimeGlobalContext GRuntimeGlobalContext;
