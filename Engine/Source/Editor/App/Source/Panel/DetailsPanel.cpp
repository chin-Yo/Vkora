#include "Panel/DetailsPanel.hpp"

#include <imgui.h>

#include "UIManage/EditorGlobalContext.hpp"


DetailsPanel::DetailsPanel()
{
    PanelName = "Details";
}

DetailsPanel::~DetailsPanel()
{
}

void DetailsPanel::OnUIRender()
{
    if (!ImGui::Begin("Details", &Enabled))
    {
        return;
    }
    if (auto* node = GEditorGlobalContext.selectedNode)
    {
        DisplaySelectedNode(node);
        float center = ImGui::GetWindowSize().x * 0.5f;
        ImGui::SetCursorPosX(center - 60);
        if (ImGui::Button("AddComponent", ImVec2(120, 0)))
        {
        }
    }
    ImGui::End();
}

void DetailsPanel::DisplaySelectedNode(scene::Node* node)
{
    auto& transform = node->GetTransform();
    DrawTransformInspector(transform);
}

void DetailsPanel::DrawComponentSelector()
{
}


void DetailsPanel::DrawTransformInspector(scene::Transform& transform)
{
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
    {
        // --- Translation ---
        ImGui::Text("Position");
        ImGui::SameLine();

        const float itemWidth = (ImGui::GetContentRegionAvail().x - 2 * ImGui::GetStyle().ItemInnerSpacing.x) / 3.0f;

        glm::vec3 pos = transform.GetTranslation();

        // X (Red)
        ImGui::PushID("PositionX");
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.72f, 0.27f, 0.27f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.64f, 0.24f, 0.24f, 1.0f));
        ImGui::PushItemWidth(itemWidth);
        if (ImGui::DragFloat("##PX", &pos.x, 0.1f))
        {
            transform.SetTranslation(pos);
        }
        ImGui::PopItemWidth();
        ImGui::PopStyleColor(3);
        ImGui::PopID();

        ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);

        // Y (Green)
        ImGui::PushID("PositionY");
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.27f, 0.72f, 0.27f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.24f, 0.64f, 0.24f, 1.0f));
        ImGui::PushItemWidth(itemWidth);
        if (ImGui::DragFloat("##PY", &pos.y, 0.1f))
        {
            transform.SetTranslation(pos);
        }
        ImGui::PopItemWidth();
        ImGui::PopStyleColor(3);
        ImGui::PopID();

        ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);

        // Z (Blue)
        ImGui::PushID("PositionZ");
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.3f, 0.5f, 1.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.27f, 0.45f, 0.9f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.24f, 0.4f, 0.8f, 1.0f));
        ImGui::PushItemWidth(itemWidth);
        if (ImGui::DragFloat("##PZ", &pos.z, 0.1f))
        {
            transform.SetTranslation(pos);
        }
        ImGui::PopItemWidth();
        ImGui::PopStyleColor(3);
        ImGui::PopID();

        // --- Rotation (via Euler angles in degrees) ---
        ImGui::Text("Rotation");
        ImGui::SameLine();

        glm::vec3 eulerDeg = scene::Transform::QuatToEulerDegrees(transform.GetRotation());

        // X (Red)
        ImGui::PushID("RotationX");
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.72f, 0.27f, 0.27f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.64f, 0.24f, 0.24f, 1.0f));
        ImGui::PushItemWidth(itemWidth);
        if (ImGui::DragFloat("##RX", &eulerDeg.x, 0.5f, -180.0f, 180.0f, "%.1f°"))
        {
            transform.SetRotation(scene::Transform::EulerDegreesToQuat(eulerDeg));
        }
        ImGui::PopItemWidth();
        ImGui::PopStyleColor(3);
        ImGui::PopID();

        ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);

        // Y (Green)
        ImGui::PushID("RotationY");
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.27f, 0.72f, 0.27f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.24f, 0.64f, 0.24f, 1.0f));
        ImGui::PushItemWidth(itemWidth);
        if (ImGui::DragFloat("##RY", &eulerDeg.y, 0.5f, -180.0f, 180.0f, "%.1f°"))
        {
            transform.SetRotation(scene::Transform::EulerDegreesToQuat(eulerDeg));
        }
        ImGui::PopItemWidth();
        ImGui::PopStyleColor(3);
        ImGui::PopID();

        ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);

        // Z (Blue)
        ImGui::PushID("RotationZ");
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.3f, 0.5f, 1.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.27f, 0.45f, 0.9f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.24f, 0.4f, 0.8f, 1.0f));
        ImGui::PushItemWidth(itemWidth);
        if (ImGui::DragFloat("##RZ", &eulerDeg.z, 0.5f, -180.0f, 180.0f, "%.1f°"))
        {
            transform.SetRotation(scene::Transform::EulerDegreesToQuat(eulerDeg));
        }
        ImGui::PopItemWidth();
        ImGui::PopStyleColor(3);
        ImGui::PopID();

        // --- Scale ---
        ImGui::Text("  Scale ");
        ImGui::SameLine();

        glm::vec3 scale = transform.GetScale();

        // X (Red)
        ImGui::PushID("ScaleX");
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.72f, 0.27f, 0.27f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.64f, 0.24f, 0.24f, 1.0f));
        ImGui::PushItemWidth(itemWidth);
        if (ImGui::DragFloat("##SX", &scale.x, 0.01f, 0.01f, 100.0f, "%.2f"))
        {
            transform.SetScale(scale);
        }
        ImGui::PopItemWidth();
        ImGui::PopStyleColor(3);
        ImGui::PopID();

        ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);

        // Y (Green)
        ImGui::PushID("ScaleY");
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.27f, 0.72f, 0.27f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.24f, 0.64f, 0.24f, 1.0f));
        ImGui::PushItemWidth(itemWidth);
        if (ImGui::DragFloat("##SY", &scale.y, 0.01f, 0.01f, 100.0f, "%.2f"))
        {
            transform.SetScale(scale);
        }
        ImGui::PopItemWidth();
        ImGui::PopStyleColor(3);
        ImGui::PopID();

        ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);

        // Z (Blue)
        ImGui::PushID("ScaleZ");
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.3f, 0.5f, 1.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.27f, 0.45f, 0.9f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.24f, 0.4f, 0.8f, 1.0f));
        ImGui::PushItemWidth(itemWidth);
        if (ImGui::DragFloat("##SZ", &scale.z, 0.01f, 0.01f, 100.0f, "%.2f"))
        {
            transform.SetScale(scale);
        }
        ImGui::PopItemWidth();
        ImGui::PopStyleColor(3);
        ImGui::PopID();

        // Optional: Reset
        if (ImGui::Button("Reset"))
        {
            transform.SetTranslation(glm::vec3(0.0f));
            transform.SetRotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
            transform.SetScale(glm::vec3(1.0f));
        }

        ImGui::Spacing();
    }
}
