#pragma once
#include <imgui.h>
#include <ImGuizmo.h>
#include <vector>
#include <volk.h>
#include <eventpp/callbacklist.h>

#include "EditorInterface/Panel.hpp"

namespace scene
{
    class Node;
}

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
    void HandleGuizmoInput();
    void DrawGuizmoToolbar();
    void HandleCameraInput();
    
private:
    bool DrawGuizmo(scene::Node* node, ImVec2 imagePos, ImVec2 imageSize);

    std::vector<VkDescriptorSet> ViewportDescriptorSets;
    eventpp::CallbackList<void(const ImVec2& PortSize)> OnViewportChange;
    ImVec2 ViewportSize{0, 0};
    bool ViewportResized = false;
    vkb::Sampler* OffScreenSampler = nullptr;
    VkRenderPass render_pass{VK_NULL_HANDLE};
    ImGuizmo::OPERATION GizmoOperation;
    ImGuizmo::MODE GizmoMode;
    bool bIsCameraControllable;
    float moveSpeed = 0.1f;
};
