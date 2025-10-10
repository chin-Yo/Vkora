#pragma once
#include <imgui.h>
#include <vector>
#include <volk.h>
#include <eventpp/callbacklist.h>

#include "EditorInterface/Panel.hpp"

namespace vkb
{
    class Sampler;
}

class ViewportPanel : public Panel
{
public:
    ViewportPanel();
    virtual ~ViewportPanel() override;

    void OnUIRender() override;

private:
    std::vector<VkDescriptorSet> ViewportDescriptorSets;
    eventpp::CallbackList<void(const ImVec2& PortSize)> OnViewportChange;
    ImVec2 ViewportSize{0, 0};
    bool ViewportResized = false;
    vkb::Sampler* OffScreenSampler = nullptr;
    VkRenderPass render_pass{VK_NULL_HANDLE};
};
