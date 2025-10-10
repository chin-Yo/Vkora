#include "Panel/Viewport.hpp"

#include "GlobalContext.hpp"
#include "backends/imgui_impl_vulkan.h"
#include "Framework/Core/Sampler.hpp"
#include "Logging/Logger.hpp"
#include "Render/RenderSystem.hpp"


ViewportPanel::ViewportPanel()
{
    vkb::VulkanDevice& device = GRuntimeGlobalContext.renderSystem->GetRenderContext().get_device();
    OffScreenSampler = new vkb::Sampler({device, vks::initializers::samplerCreateInfo()});
    ViewportDescriptorSets.resize(3);
}

ViewportPanel::~ViewportPanel()
{
    delete OffScreenSampler;

    for (uint32_t i = 0; i < ViewportDescriptorSets.size(); i++)
    {
        if (ViewportDescriptorSets[i])
            ImGui_ImplVulkan_RemoveTexture(ViewportDescriptorSets[i]);
    }
}

void ViewportPanel::OnUIRender()
{
    ImGui::SetNextWindowSize(ImVec2(500, 600), ImGuiCond_Once);
    ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::Text("Viewport fuck vulkan");
    ImVec2 currentViewportSize = ImGui::GetContentRegionAvail();

    bool hasChanged = (std::abs(ViewportSize.x - currentViewportSize.x) > 2) ||
        (std::abs(ViewportSize.y - currentViewportSize.y) > 2);
    if (currentViewportSize.x > 0.0f && currentViewportSize.y > 0.0f && hasChanged)
    {
        ViewportSize = currentViewportSize;
        ViewportResized = true;

        LOG_INFO("Viewport resized to {} x {}", ViewportSize.x, ViewportSize.y)
        GRuntimeGlobalContext.renderSystem->ResetViewportRTs(ViewportSize, OffScreenSampler, ViewportDescriptorSets);
        OnViewportChange(currentViewportSize);
    }
    auto index = GRuntimeGlobalContext.renderSystem->GetRenderContext().get_active_frame_index();
    if (ViewportDescriptorSets[index] != nullptr)
    {
        ImGui::Image(ViewportDescriptorSets[index], ViewportSize);
    }

    ImGui::End();
}
