#include "Panel/DetailsPanel.hpp"

#include <imgui.h>

#include "Engine/Asset/AssetRegistry.hpp"
#include "Engine/SceneGraph/ComponentPool.hpp"
#include "Engine/SceneGraph/Components/SubMesh.hpp"
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
        DrawComponentSelector(node);
    }
    ImGui::End();
}

void DetailsPanel::DisplaySelectedNode(scene::Node* node)
{
    auto& transform = node->GetTransform();
    DrawTransformInspector(transform);

    auto* scene = node->GetScene();
    auto& handles = node->GetComponentHandles();
    for (auto handle : handles)
    {
        if (handle.type == rttr::type::get<scene::SubMesh>())
        {
            auto* subMesh = scene->GetComponentManager()->GetComponentFormNode<scene::SubMesh>(node->GetID());
            if (ImGui::CollapsingHeader("SubMesh", ImGuiTreeNodeFlags_DefaultOpen))
            {
                static int currentSelection = -1; // -1 表示“无选择”
                auto options = AssetRegistry::Get().GetAllAssetsOfType(AssetType::Mesh);

                const char* previewValue = "Select...";
                if (currentSelection == -1)
                {
                    previewValue = "(None)";
                }
                else if (currentSelection >= 0 && currentSelection < (int)options.size())
                {
                    previewValue = options[currentSelection]->name.c_str();
                }

                if (ImGui::BeginCombo("MeshAsset", previewValue))
                {
                    if (ImGui::Selectable("(None)", currentSelection == -1))
                    {
                        currentSelection = -1;
                    }
                    for (int i = 0; i < (int)options.size(); ++i)
                    {
                        bool isSelected = (currentSelection == i);
                        if (ImGui::Selectable(options[i]->name.c_str(), isSelected))
                        {
                            currentSelection = i;
                        }
                        if (isSelected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                    }

                    ImGui::EndCombo();
                }
            }
            if (subMesh->bHasMeshData)
            {
            }
        }
    }
}

void DetailsPanel::DrawComponentSelector(scene::Node* node)
{
    auto* scene = node->GetScene();
    float center = ImGui::GetWindowSize().x * 0.5f;
    ImGui::SetCursorPosX(center - 60);
    if (ImGui::Button("AddComponent", ImVec2(120, 0)))
    {
        ImGui::OpenPopup("AddComponentPopup");
    }
    if (ImGui::BeginPopup("AddComponentPopup"))
    {
        if (ImGui::MenuItem("SubMesh"))
        {
            scene->GetComponentManager()->AddComponent<::scene::SubMesh>(node);
        }
        if (ImGui::MenuItem("Camera"))
        {
        }
        if (ImGui::BeginMenu("Lights"))
        {
            if (ImGui::MenuItem("Point Light"))
            {
            }
            if (ImGui::MenuItem("Directional Light"))
            {
            }
            if (ImGui::MenuItem("Spot Light"))
            {
            }
            ImGui::EndMenu();
        }
        ImGui::EndPopup();
    }
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
