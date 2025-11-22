#pragma once

#include <atomic>
#include <chrono>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <volk.h>

#include "WindowSystem.hpp"
#include "Render/RenderSystem.hpp"
#include "Engine/Asset/Manager/AssetManager.hpp"
#include "World/WorldManager.hpp"
#include "Engine/SceneGraph/ComponentPool.hpp"

namespace vkb
{
    class DebugUtils;
    class Window;
    class VulkanDevice;
    class Instance;
}

struct EngineConfig
{
    int MaxFPS = 60;
    bool EnableVSync = true;
};

struct BackendOptions
{
    vkb::Window* window{nullptr};
};

class Engine
{
    static const float FPSAlpha;
    friend class Editor;

public:
    void StartEngine(const std::string& ConfigFilePath);
    void ShutdownEngine();

    void Initialize();
    void Clear();

    bool IsQuit() const { return isQuit; }
    bool TickOneFrame(float DeltaTime);

    int GetFPS() const { return FPS; }
    void SetMaxFPS(int fps) { MaxFPS = fps; }
    std::string GetEngineStatus() const;

protected:
    void LogicalTick(float DeltaTime);
    bool RendererTick(float DeltaTime);

    void CalculateFPS(float DeltaTime);
    float CalculateDeltaTime();
    void LimitFPS(float& DeltaTime);

    void SetIsIconify(bool bIsIconify);

protected:
    bool isQuit = false;
    int MaxFPS = 120;

    std::chrono::steady_clock::time_point LastTickTimePoint{std::chrono::steady_clock::now()};
    std::chrono::steady_clock::time_point FrameStartTimePoint{std::chrono::steady_clock::now()};
    float AverageDuration = 0.f;
    int FrameCount = 0;
    int FPS = 0;
    EngineConfig mConfig;

    bool bIsMinimized = false;

    struct VkBackend
    {
        std::unique_ptr<vkb::Instance> instance;
        std::unique_ptr<vkb::VulkanDevice> device;
        vkb::VulkanDevice const& GetDevice() const { return *device; }
        vkb::VulkanDevice& GetDevice() { return *device; }
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        /** @brief Set of device extensions to be enabled for this example and whether they are optional (must be set in the derived constructor) */
        std::unordered_map<const char*, bool> device_extensions;
        /** @brief Set of instance extensions to be enabled for this example and whether they are optional (must be set in the derived constructor) */
        std::unordered_map<const char*, bool> instance_extensions;
        /** @brief Set of instance layers to be enabled for this example and whether they are optional (must be set in the derived constructor) */
        std::unordered_map<const char*, bool> instance_layers;
        /** @brief Vector of layer settings to be enabled for this example (must be set in the derived constructor) */
        std::vector<VkLayerSettingEXT> layer_settings;
        /** @brief The Vulkan API version to request for this sample at instance creation time */
        uint32_t api_version = VK_API_VERSION_1_1;
        /** @brief Whether or not we want a high priority graphics queue. */
        bool high_priority_graphics_queue{false};
        std::unique_ptr<vkb::DebugUtils> debug_utils;
        /**
        * @brief Add a sample-specific device extension
        * @param extension The extension name
        * @param optional (Optional) Whether the extension is optional
        */
        void AddDeviceExtension(const char* extension, bool optional = false);
        /**
         * @brief Add a sample-specific instance extension
         * @param extension The extension name
         * @param optional (Optional) Whether the extension is optional
         */
        void AddInstanceExtension(const char* extension, bool optional = false);
        /**
         * @brief Add a sample-specific instance layer
         * @param layer The layer name
         * @param optional (Optional) Whether the extension is optional
         */
        void AddInstanceLayer(const char* layer, bool optional = false);
        /**
         * @brief Add a sample-specific layer setting
         * @param layerSetting The layer setting
         */
        void AddLayerSetting(VkLayerSettingEXT const& layerSetting);
    } RenderBackend;

    void InitRenderBackend(const BackendOptions& options);
    void ShutdownRenderBackend();
    std::unique_ptr<WindowSystem> windowSystem;
    std::unique_ptr<WorldManager> worldManager;
    std::unique_ptr<RenderSystem> renderSystem;
    std::unique_ptr<AssetManager> assetManager;
};
