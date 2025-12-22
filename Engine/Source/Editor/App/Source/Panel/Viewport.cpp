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
#include "Rendering/RenderSystem.hpp"
#include "UIManage/EditorGlobalContext.hpp"
#include "World/WorldManager.hpp"
#include "IconsFontAwesome5.h"
#include "Engine/Engine.hpp"

ViewportPanel::ViewportPanel()
{
    vkb::VulkanDevice& device = GRuntimeGlobalContext.renderSystem->GetDevice();
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
    if (!ImGui::Begin(ICON_FA_VIDEO " Viewport", nullptr,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
    {
        ImGui::End();
        return;
    }
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
        bool bIsGuizmo = DrawGuizmo(GEditorGlobalContext.selectedNode, imagePos, imageSize);
        if (!bIsGuizmo)
            HandleCameraInput();
    }

    ImGui::End();
}

void ViewportPanel::HandleGuizmoInput()
{
    // Switch operating mode by pressing Q/W/E/R
    if (ImGui::IsWindowFocused())
    {
        if (ImGui::IsKeyPressed(ImGuiKey_L))
        {
            GizmoOperation = ImGuizmo::TRANSLATE;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_R))
        {
            GizmoOperation = ImGuizmo::ROTATE;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_C))
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

void ViewportPanel::HandleCameraInput()
{
    static ImVec2 g_ClickPos;
    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        auto* Win = GRuntimeGlobalContext.windowSystem->GetWindow();

        // 记录点击位置（使用ImGui坐标，即窗口内坐标）
        g_ClickPos = ImGui::GetIO().MousePos;

        // 激活控制模式
        bIsCameraControllable = true;
        ImGui::SetWindowFocus();

        // 隐藏光标（使用HIDDEN模式，保留位置控制能力）
        glfwSetInputMode(Win, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

        // 立即冻结光标位置（关键！）
        glfwSetCursorPos(Win, g_ClickPos.x, g_ClickPos.y);
        return; // 本帧不处理移动，避免初始跳变
    }

    // 2. 仅在控制状态下处理输入
    if (!bIsCameraControllable)
        return;

    auto* Win = GRuntimeGlobalContext.windowSystem->GetWindow();
    auto* Camera = GRuntimeGlobalContext.worldManager->GetViewportCamera();
    auto* CameraNode = Camera->GetOwner();
    auto& NodeTransform = CameraNode->GetTransform();

    // 获取图像区域（用于边界检测）
    ImVec2 imageRegionMin = ImGui::GetItemRectMin();
    ImVec2 imageRegionMax = ImGui::GetItemRectMax();

    // 检查焦点和区域有效性
    bool shouldHandleInput =
        ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
        ImGui::IsMouseHoveringRect(imageRegionMin, imageRegionMax, false);

    if (shouldHandleInput)
    {
        const float moveSpeed = 0.1f;

        // --- 键盘平移控制 ---
        glm::vec3 translation(0.0f);
        if (ImGui::IsKeyDown(ImGuiKey_W)) translation += NodeTransform.GetForward();
        if (ImGui::IsKeyDown(ImGuiKey_S)) translation -= NodeTransform.GetForward();
        if (ImGui::IsKeyDown(ImGuiKey_A)) translation -= NodeTransform.GetRight();
        if (ImGui::IsKeyDown(ImGuiKey_D)) translation += NodeTransform.GetRight();
        if (ImGui::IsKeyDown(ImGuiKey_E)) translation += NodeTransform.GetUp();
        if (ImGui::IsKeyDown(ImGuiKey_Q)) translation -= NodeTransform.GetUp();

        if (glm::length(translation) > 0.01f)
        {
            translation = glm::normalize(translation) * moveSpeed;
            NodeTransform.SetTranslation(translation);
        }

        // --- 鼠标旋转控制（使用原生GLFW位置）---
        double currX, currY;
        glfwGetCursorPos(Win, &currX, &currY); // 获取真实物理位置

        // 计算相对于点击点的偏移（关键：使用物理位置而非ImGui位置）
        float deltaX = static_cast<float>(currX - g_ClickPos.x);
        float deltaY = static_cast<float>(currY - g_ClickPos.y);

        if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            const float rotateSpeed = 0.002f;
            auto euler = NodeTransform.GetRotationEuler();

            // 应用旋转（注意坐标系转换）
            euler.z += deltaX * rotateSpeed; // Yaw (around Z-axis)
            euler.y = glm::clamp(euler.y - deltaY * rotateSpeed, -1.5f, 1.5f); // Pitch (around Y-axis)

            NodeTransform.SetRotation(euler);
            */
            // 构造增量旋转：绕本地 X 轴（俯仰）和世界 Y 轴（偏航）
            glm::quat deltaPitch = glm::angleAxis(glm::radians(-deltaY * rotateSpeed), glm::vec3(1, 0, 0));
            glm::quat deltaYaw = glm::angleAxis(glm::radians(-deltaX * rotateSpeed), glm::vec3(0, 1, 0));

            // 先应用 yaw（绕世界 Y），再应用 pitch（绕旋转后的本地 X）
            // 注意顺序：右乘 = 局部旋转
            glm::quat rotation = deltaYaw * NodeTransform.GetRotation() * deltaPitch;
            NodeTransform.SetRotation(rotation);
        }

        // ✅ 每帧冻结光标位置（关键！）
        glfwSetCursorPos(Win, g_ClickPos.x, g_ClickPos.y);
    }
    else
    {
        // 失去焦点时退出控制
        bIsCameraControllable = false;
    }

    // 3. 检测释放控制
    if (bIsCameraControllable && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
        bIsCameraControllable = false;

        // 恢复光标可见性
        glfwSetInputMode(Win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

        // ✅ 在点击位置精确恢复光标
        glfwSetCursorPos(Win, g_ClickPos.x, g_ClickPos.y);
    }
}

bool ViewportPanel::DrawGuizmo(scene::Node* node, ImVec2 imagePos, ImVec2 imageSize)
{
    if (!node)
        return false;

    auto* camera = GRuntimeGlobalContext.worldManager->GetViewportCamera();
    if (!camera || camera->GetOwner() == GEditorGlobalContext.selectedNode)
        return false;
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
        return true;
    }
    return false;
}
