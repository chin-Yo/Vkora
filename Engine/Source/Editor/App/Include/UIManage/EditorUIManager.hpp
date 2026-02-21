#pragma once
#define IMGUI_DEFINE_DOCKING
#include <imgui.h>
#include <memory>
#include <vector>
#include <volk.h>

#include "Panel.hpp"
#include "Framework/Core/DescriptorPool.hpp"


namespace vkb
{
    class Sampler;
    class VulkanDevice;
}

class EditorUIInterface
{
public:
    EditorUIInterface() = default;
    virtual ~EditorUIInterface() = default;

    virtual void Initialize() = 0;
    virtual void Prepare(VkRenderPass renderPass, VkQueue queue, uint32_t MinImageCount,
                         uint32_t ImageCount) = 0;
    virtual void Shutdown() = 0;

    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;

    virtual void RenderUI() = 0;
};

class EditorUIManager : public EditorUIInterface
{
public:
    EditorUIManager(vkb::VulkanDevice& device);
    ~EditorUIManager() override;
    void Initialize() override;
    void Prepare(VkRenderPass renderPass, VkQueue queue, uint32_t MinImageCount,
                 uint32_t ImageCount) override;
    void Shutdown() override;
    void BeginFrame() override;
    void EndFrame() override;
    void RenderUI() override;

private:
    vkb::VulkanDevice& device;
    std::vector<std::shared_ptr<Panel>> EditorPanels;

    VkDescriptorPool descriptorPool;
    VkRenderPass render_pass;
};
