#include "Panel/Viewport.hpp"
#define IMGUIZMO_USE_GLM
#include <ImGuizmo.h>
#include <imgui_internal.h>
#include <glm/gtc/type_ptr.hpp>

#include "GlobalContext.hpp"
#include "backends/imgui_impl_vulkan.h"
#include "Engine/SceneGraph/Node.hpp"
#include "Engine/SceneGraph/Components/PerspectiveCamera.hpp"
#include "Framework/Core/Sampler.hpp"
#include "Logging/Logger.hpp"
#include "Render/RenderSystem.hpp"
#include "UIManage/EditorGlobalContext.hpp"
#include "World/WorldManager.hpp"


ViewportPanel::ViewportPanel()
{
    vkb::VulkanDevice& device = GRuntimeGlobalContext.renderSystem->GetRenderContext().get_device();
    OffScreenSampler = new vkb::Sampler({device, vks::initializers::samplerCreateInfo()});
    ViewportDescriptorSets.resize(3);

    GizmoOperation = ImGuizmo::TRANSLATE;
    GizmoMode = ImGuizmo::WORLD;
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
    HandleGuizmoInput();
    DrawGuizmoToolbar();
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
        ImVec2 imagePos = ImGui::GetItemRectMin();
        ImVec2 imageSize = ImGui::GetItemRectSize();
        DrawGuizmo(GEditorGlobalContext.selectedNode, imagePos, imageSize);
    }

    ImGui::End();
}

void ViewportPanel::HandleGuizmoInput()
{
    // Switch operating mode by pressing Q/W/E/R
    if (ImGui::IsWindowFocused())
    {
        if (ImGui::IsKeyPressed(ImGuiKey_Q))
        {
            GizmoOperation = ImGuizmo::TRANSLATE;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_E))
        {
            GizmoOperation = ImGuizmo::ROTATE;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_R))
        {
            GizmoOperation = ImGuizmo::SCALE;
        }
        
        if (ImGui::IsKeyPressed(ImGuiKey_T))
        {
            GizmoMode = (GizmoMode == ImGuizmo::LOCAL) ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
        }
    }
}

void ViewportPanel::DrawGuizmoToolbar()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 2));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    
    auto& colors = ImGui::GetStyle().Colors;
    const auto& buttonHovered = colors[ImGuiCol_ButtonHovered];
    const auto& buttonActive = colors[ImGuiCol_ButtonActive];
    
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(buttonHovered.x, buttonHovered.y, buttonHovered.z, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(buttonActive.x, buttonActive.y, buttonActive.z, 0.5f));

    float lineHeight = GImGui->Font->LegacySize + GImGui->Style.FramePadding.y * 2.0f;
    ImVec2 buttonSize = {lineHeight + 3.0f, lineHeight};

    // ✅ 平移按钮
    bool isTranslate = GizmoOperation == ImGuizmo::TRANSLATE;
    if (isTranslate)
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
    
    if (ImGui::Button("T", buttonSize))
        GizmoOperation = ImGuizmo::TRANSLATE;
    
    if (isTranslate)
        ImGui::PopStyleColor();
    
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Translate (Q)");

    ImGui::SameLine();

    // ✅ 旋转按钮
    bool isRotate = GizmoOperation == ImGuizmo::ROTATE;
    if (isRotate)
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
    
    if (ImGui::Button("R", buttonSize))
        GizmoOperation = ImGuizmo::ROTATE;
    
    if (isRotate)
        ImGui::PopStyleColor();
    
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Rotate (E)");

    ImGui::SameLine();

    // ✅ 缩放按钮
    bool isScale = GizmoOperation == ImGuizmo::SCALE;
    if (isScale)
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
    
    if (ImGui::Button("S", buttonSize))
        GizmoOperation = ImGuizmo::SCALE;
    
    if (isScale)
        ImGui::PopStyleColor();
    
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Scale (R)");

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();

    // ✅ 坐标系切换按钮
    bool isLocal = GizmoMode == ImGuizmo::LOCAL;
    if (ImGui::Button(isLocal ? "Local" : "World", ImVec2(60, buttonSize.y)))
        GizmoMode = isLocal ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
    
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Toggle Coordinate System (T)");

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(2);
}

void ViewportPanel::DrawGuizmo(scene::Node* node, ImVec2 imagePos, ImVec2 imageSize)
{
    if (!node)
        return;

    auto* camera = GRuntimeGlobalContext.worldManager->GetViewportCamera();
    if (!camera)
        return;
    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(imagePos.x, imagePos.y, imageSize.x, imageSize.y);

    glm::mat4 view = camera->GetView();
    glm::mat4 projection = camera->GetProjection();
    glm::mat4 model = node->GetTransform().GetMatrix();

#ifndef IMGUIZMO_USE_GLM
    // If GLM support is not enabled, transposition is required.
    view = glm::transpose(view);
    projection = glm::transpose(projection);
    model = glm::transpose(model);
#endif

    ImGuizmo::Manipulate(
        glm::value_ptr(view),
        glm::value_ptr(projection),
        GizmoOperation,
        GizmoMode,
        glm::value_ptr(model)
    );

    if (ImGuizmo::IsUsing())
    {
#ifndef IMGUIZMO_USE_GLM
        model = glm::transpose(model);
#endif

        node->GetTransform().SetMatrix(model);
    }
}
