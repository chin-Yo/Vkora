#pragma once

#include <imgui.h>
#include <volk.h>

#include "EditorInterface/EditorUIManager.hpp"
#include "Framework/Core/Instance.hpp"
#include "Framework/Core/VulkanDevice.hpp"
#include "Framework/Rendering/RenderContext.hpp"
#include "Framework/Rendering/RenderPipeline.hpp"


class AssetManager;

namespace scene
{
    class Camera;
    class Scene;
}

namespace vkb
{
    class Sampler;
}

namespace vkb::sg
{
    class PerspectiveCamera;
}

class RenderSystem
{
public:
    RenderSystem(vkb::Window* window, vkb::VulkanDevice* device, VkSurfaceKHR surface);
    ~RenderSystem();

public:
    bool Prepare();

    bool RenderPrepare();

    void RequestGpuFeatures(vkb::PhysicalDevice& gpu);
    // entrance
    void Draw(vkb::CommandBuffer& command_buffer, vkb::RenderTarget& render_target);
    void Render(vkb::CommandBuffer& command_buffer);
    // pivotal
    void DrawRenderpass(vkb::CommandBuffer& command_buffer, vkb::RenderTarget& render_target);
    void Update(float delta_time);
    void UpdateDebugWindow();
    void Finish();

    void SetViewportAndScissor(vkb::CommandBuffer const& command_buffer, VkExtent2D const& extent);

    void CreateRenderContext();
    void CreateRenderContext_Impl(const std::vector<VkSurfaceFormatKHR>& surface_priority_list);

    void ResetStatsView();

    bool Resize(uint32_t width, uint32_t height);
    void SetRenderContext(std::unique_ptr<vkb::RenderContext>&& rc);
    void SetRenderPipeline(std::unique_ptr<vkb::RenderPipeline>&& rp);

    void InitializeUIRenderBackend(EditorUIInterface* UIManager);


    std::unique_ptr<vkb::RenderPipeline> CreateOneRenderpassTwoSubpasses(scene::Scene& scene, scene::Camera& camera);
    VkFormat albedo_format{VK_FORMAT_R8G8B8A8_UNORM};
    VkFormat normal_format{VK_FORMAT_A2B10G10R10_UNORM_PACK32};
    VkImageUsageFlags rt_usage_flags{VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT};
    bool OffScreenResourcesReady = false;
    std::unique_ptr<vkb::RenderTarget> CreateRenderTarget(ImVec2 size);
    std::unique_ptr<vkb::RenderTarget> CreateShadowMap(ImVec2 size);

    void ResetViewportRTs(ImVec2& size, vkb::Sampler* sampler, std::vector<VkDescriptorSet>& ViewportDescriptorSets);
    std::vector<std::unique_ptr<vkb::RenderTarget>> ViewportRTs;

    //void ViewportResize(ImVec2 size);

    void DrawPipeline(vkb::CommandBuffer& command_buffer,
                      vkb::RenderTarget& render_target,
                      vkb::RenderPipeline& render_pipeline);

    // Shadow map
    std::unique_ptr<vkb::RenderPipeline> CreateShadowMapRenderPass();
    void DrawShadowPass(vkb::CommandBuffer& command_buffer, vkb::RenderTarget& render_target);
    std::unique_ptr<vkb::RenderPipeline> ShadowPipeline;
    std::unique_ptr<vkb::RenderTarget> ShadowRenderTarget;

private: // -----------------Member
    vkb::Window* window{nullptr};

    vkb::VulkanDevice* device;

    VkSurfaceKHR surface = VK_NULL_HANDLE;


    /**
     * @brief Context used for rendering, it is responsible for managing the frames and their underlying images
     */
    std::unique_ptr<vkb::RenderContext> render_context;

    /**
     * @brief Pipeline used for rendering, it should be set up by the concrete sample
     */
    std::unique_ptr<vkb::RenderPipeline> render_pipeline;

    //std::unique_ptr<EditorUIManager> EditorUI;
    std::unique_ptr<vkb::RenderPass> EditorUIRenderpass;

    EditorUIInterface* UIManager = nullptr;
    // std::unique_ptr<vkb::stats::Stats> stats;

    static constexpr float STATS_VIEW_RESET_TIME{10.0f}; // 10 seconds
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


public:
    vkb::VulkanDevice const& GetDevice() const { return *device; }
    vkb::VulkanDevice& GetDevice() { return *device; }
    vkb::RenderContext const& GetRenderContext() const { return *render_context; }
    vkb::RenderContext& GetRenderContext() { return *render_context; }
    vkb::RenderPipeline const& GetRenderPipeline() const { return *render_pipeline; }
    vkb::RenderPipeline& GetRenderPipeline() { return *render_pipeline; }
    //Configuration get_configuration()
    std::vector<VkSurfaceFormatKHR> const& GetSurfacePriorityList() const;
    std::vector<VkSurfaceFormatKHR>& GetSurfacePriorityList();
};
