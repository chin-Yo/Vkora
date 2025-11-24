#include "Panel/DetailsPanel.hpp"

#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <imgui_internal.h>

#include "GlobalContext.hpp"
#include "Drawer/ImageSelector.hpp"
#include "Drawer/Refl_Drawer.hpp"
#include "Engine/Asset/AssetRegistry.hpp"
#include "Engine/Asset/Import/ModelLoader.hpp"
#include "Engine/Asset/Manager/AssetManager.hpp"
#include "Engine/SceneGraph/ComponentPool.hpp"
#include "Engine/SceneGraph/Components/Light.hpp"
#include "Engine/SceneGraph/Components/Material.hpp"
#include "Engine/SceneGraph/Components/Pbr_Material.hpp"
#include "Engine/SceneGraph/Components/SubMesh.hpp"
#include "Engine/SceneGraph/Components/Texture.hpp"
#include "Misc/Paths.hpp"
#include "Render/RenderSystem.hpp"
#include "UIManage/EditorGlobalContext.hpp"
#include "Logging/Logger.hpp"

DetailsPanel::DetailsPanel()
{
    PanelName = "Details";
}

DetailsPanel::~DetailsPanel()
{
}

void DetailsPanel::OnUIRender()
{
    if (!ImGui::Begin(ICON_FA_INFO_CIRCLE " Details", &Enabled))
    {
        ImGui::End();
        return;
    }
    if (auto* node = GEditorGlobalContext.selectedNode)
    {
        DisplaySelectedNode(node);
        DrawComponentSelector(node);
        DrawDuplicateComponentModal();
    }
    ImGui::End();
}

void DetailsPanel::DisplaySelectedNode(scene::Node* node)
{
    if (!node)
    {
        LastSelectedNode = nullptr;
        return;
    }
    if (node != LastSelectedNode)
    {
        // 如果是新选择的节点，就将它的名字复制到我们的缓冲区
        // 使用 strncpy 来防止缓冲区溢出
        const std::string& nodeName = node->GetName(); // 假设 GetName() 返回 std::string
        const size_t copyLength = std::min(nodeName.length(), sizeof(NodeNameBuffer) - 1);
        std::copy_n(nodeName.begin(), copyLength, NodeNameBuffer);
        NodeNameBuffer[copyLength] = '\0';
        LastSelectedNode = node;
    }
    if (ImGui::InputText("Node Name", NodeNameBuffer, sizeof(NodeNameBuffer)))
    {
        node->SetName(NodeNameBuffer);
    }

    ImGui::Separator();

    auto& transform = node->GetTransform();
    DrawTransformInspector(transform);

    auto* scene = node->GetScene();
    auto& handles = node->GetComponentHandles();
    for (auto handle : handles)
    {
        if (handle.type == rttr::type::get<scene::SubMesh>())
        {
            auto* subMesh = scene->GetComponentManager()->GetComponentFormNode<scene::SubMesh>(node->GetID());
            ImGui::PushID(subMesh);
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
                            // Execute once when selected, within one frame
                            currentSelection = i;

                            auto* aM = GRuntimeGlobalContext.assetManager;
                            auto meshData = aM->GetMesh(options[i]->relativePath);
                            if (meshData)
                            {
                                subMesh->SetMeshData(meshData);
                                scene::Material* material = new scene::PBRMaterial("Default");
                                subMesh->set_material(*material);
                            }
                        }
                        if (isSelected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                    }

                    ImGui::EndCombo();
                }
                auto* material = subMesh->get_mut_material();
                ui::ImageSelector::Draw("base color texture", material->base_color_texture);
                ui::ImageSelector::Draw("normal texture", material->normal_texture);
                ui::ImageSelector::Draw("metallic texture", material->metallic_texture);
                ui::ImageSelector::Draw("roughness texture", material->roughness_texture);
                ui::ImageSelector::Draw("ao texture", material->ao_texture);
            }
            ImGui::PopID();
        }
        else if (handle.type == rttr::type::get<scene::Light>())
        {
            auto* light = scene->GetComponentManager()->GetComponentFormNode<scene::Light>(node->GetID());
            if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ui::PropertyDrawer::DrawObject(light);
            }
        }
    }
}

void DetailsPanel::DrawComponentSelector(scene::Node* node)
{
    auto* scene = node->GetScene();
    bool bIsRepeat = false;
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
            if (!node->HasComponent(rttr::type::get<::scene::SubMesh>()))
            {
                auto* subMesh = scene->GetComponentManager()->AddComponent<::scene::SubMesh>(node);
                subMesh->bHasMeshData = false;
            }
            else
            {
                bIsRepeat = true;
            }
        }
        if (ImGui::MenuItem("Camera"))
        {
        }
        if (ImGui::BeginMenu("Lights"))
        {
            if (ImGui::MenuItem("Point Light"))
            {
                auto* light = scene->GetComponentManager()->AddComponent<::scene::Light>(node);
                light->set_light_type(::scene::LightType::Point);
            }
            if (ImGui::MenuItem("Directional Light"))
            {
                auto* light = scene->GetComponentManager()->AddComponent<::scene::Light>(node);
                light->set_light_type(::scene::LightType::Directional);
            }
            if (ImGui::MenuItem("Spot Light"))
            {
                auto* light = scene->GetComponentManager()->AddComponent<::scene::Light>(node);
                light->set_light_type(::scene::LightType::Spot);
            }
            ImGui::EndMenu();
        }
        ImGui::EndPopup();
    }
    if (bIsRepeat)
    {
        ShowDuplicateComponentError(rttr::type::get<scene::SubMesh>());
        bIsRepeat = false;
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

void DetailsPanel::ShowDuplicateComponentError(const rttr::type& type)
{
    PendingDuplicateComponentType = type;
    ImGui::OpenPopup("Component type already exists");
#ifdef DEBUG
    ImGuiContext& g = *GImGui;
    LOG_INFO("Current popup stack size: {}", g.OpenPopupStack.Size)
    for (int i = 0; i < g.OpenPopupStack.Size; ++i)
    {
        LOG_INFO("  [{}] ID: 0x{}, Name: {}", i, (uint32_t)g.OpenPopupStack[i].PopupId,
                 (uint32_t)g.OpenPopupStack[i].PopupId)
    }
#endif
}

void DetailsPanel::DrawDuplicateComponentModal()
{
    if (!PendingDuplicateComponentType.has_value())
        return;

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Component type already exists", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("The component type: %s already exists and cannot be added again!",
                    PendingDuplicateComponentType->get_name().data());
        ImGui::Separator();

        if (ImGui::Button("OK", ImVec2(120, 0)) || ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
            PendingDuplicateComponentType.reset();
        }
        ImGui::SetItemDefaultFocus();

        ImGui::EndPopup();
    }
    else
    {
        PendingDuplicateComponentType.reset();
    }
}
