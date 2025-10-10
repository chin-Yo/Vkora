#pragma once

#include <imgui.h>
#include <volk.h>

#include "EditorUI.hpp"
#include "EditorInterface/EditorUIManager.hpp"
#include "Framework/Core/Instance.hpp"
#include "Framework/Core/VulkanDevice.hpp"
#include "Framework/Rendering/RenderContext.hpp"
#include "Framework/Rendering/RenderPipeline.hpp"


namespace vkb
{
    class Sampler;
}

namespace vkb::sg
{
    class Scene;
    class PerspectiveCamera;
}

struct ApplicationOptions
{
    bool benchmark_enabled{false};
    vkb::Window* window{nullptr};
};

class RenderSystem
{
public:
    RenderSystem() = default;
    ~RenderSystem();

public:
    bool Prepare(const ApplicationOptions& options);

    void RequestGpuFeatures(vkb::PhysicalDevice& gpu);

    std::unique_ptr<vkb::Instance> CreateInstance();

    std::unique_ptr<vkb::VulkanDevice> CreateDevice(vkb::PhysicalDevice& gpu);
    // entrance
    void Draw(vkb::CommandBuffer& command_buffer, vkb::RenderTarget& render_target);
    void Render(vkb::CommandBuffer& command_buffer);
    // pivotal
    void DrawRenderpass(vkb::CommandBuffer& command_buffer, vkb::RenderTarget& render_target);
    void Update(float delta_time);
    void UpdateScene(float delta_time);
    void UpdateDebugWindow();
    void Finish();

    void SetViewportAndScissor(vkb::CommandBuffer const& command_buffer, VkExtent2D const& extent);

    void CreateRenderContext();
    void CreateRenderContext_Impl(const std::vector<VkSurfaceFormatKHR>& surface_priority_list);

    void ResetStatsView();

    bool Resize(uint32_t width, uint32_t height);
    void SetApiVersion(uint32_t requested_api_version);
    void SetRenderContext(std::unique_ptr<vkb::RenderContext>&& rc);
    void SetRenderPipeline(std::unique_ptr<vkb::RenderPipeline>&& rp);
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

    void InitializeUIRenderBackend(EditorUIInterface* UIManager);


    std::unique_ptr<vkb::RenderPipeline> CreateOneRenderpassTwoSubpasses();
    VkFormat albedo_format{VK_FORMAT_R8G8B8A8_UNORM};
    VkFormat normal_format{VK_FORMAT_A2B10G10R10_UNORM_PACK32};
    VkImageUsageFlags rt_usage_flags{VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT};
    bool OffScreenResourcesReady = false;
    std::unique_ptr<vkb::RenderTarget> CreateRenderTarget(ImVec2 size);

    void ResetViewportRTs(ImVec2& size, vkb::Sampler* sampler, std::vector<VkDescriptorSet>& ViewportDescriptorSets);
    std::vector<std::unique_ptr<vkb::RenderTarget>> ViewportRTs;

    vkb::sg::PerspectiveCamera* camera{};
    //void ViewportResize(ImVec2 size);

    void DrawPipeline(vkb::CommandBuffer& command_buffer,
                      vkb::RenderTarget& render_target,
                      vkb::RenderPipeline& render_pipeline);

private: // -----------------Member
    vkb::Window* window{nullptr};

    /**
     * @brief The Vulkan instance
     */
    std::unique_ptr<vkb::Instance> instance;

    /**
     * @brief The Vulkan device
     */
    std::unique_ptr<vkb::VulkanDevice> device;

    /**
     * @brief Context used for rendering, it is responsible for managing the frames and their underlying images
     */
    std::unique_ptr<vkb::RenderContext> render_context;

    /**
     * @brief Pipeline used for rendering, it should be set up by the concrete sample
     */
    std::unique_ptr<vkb::RenderPipeline> render_pipeline;

    /**
     * @brief Holds all scene information
     */
    std::unique_ptr<vkb::sg::Scene> scene;

    //std::unique_ptr<EditorUIManager> EditorUI;
    std::unique_ptr<vkb::RenderPass> EditorUIRenderpass;

    EditorUIInterface* UIManager = nullptr;
    // std::unique_ptr<vkb::stats::Stats> stats;

    static constexpr float STATS_VIEW_RESET_TIME{10.0f}; // 10 seconds

    /**
     * @brief The Vulkan surface
     */
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    /**
     * @brief A list of surface formats in order of priority (vector[0] has high priority, vector[size-1] has low priority)
     */
    std::vector<VkSurfaceFormatKHR> surface_priority_list = {
        {VK_FORMAT_R8G8B8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
        {VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}
    };

    /**
     * @brief The configuration of the sample
     */
    // Configuration configuration{};

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

public:
    vkb::Instance const& GetInstance() const { return *instance; }
    vkb::Instance& GetInstance() { return *instance; }
    vkb::VulkanDevice const& GetDevice() const { return *device; }
    vkb::VulkanDevice& GetDevice() { return *device; }
    vkb::RenderContext const& GetRenderContext() const { return *render_context; }
    vkb::RenderContext& GetRenderContext() { return *render_context; }
    vkb::RenderPipeline const& GetRenderPipeline() const { return *render_pipeline; }
    vkb::RenderPipeline& GetRenderPipeline() { return *render_pipeline; }
    vkb::sg::Scene const& GetScene() const { return *scene; }
    vkb::sg::Scene& GetScene() { return *scene; }
    std::unordered_map<const char*, bool> const& GetDeviceExtensions() const;
    std::unordered_map<const char*, bool> const& GetInstanceExtensions() const;
    std::unordered_map<const char*, bool> const& GetInstanceLayers() const;
    std::vector<VkLayerSettingEXT> const& GetLayerSettings() const;
    //Configuration get_configuration()
    std::vector<VkSurfaceFormatKHR> const& GetSurfacePriorityList() const;
    std::vector<VkSurfaceFormatKHR>& GetSurfacePriorityList();
};
