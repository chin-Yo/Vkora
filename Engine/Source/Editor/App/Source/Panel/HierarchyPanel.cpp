#include "Panel/HierarchyPanel.hpp"

#include <imgui.h>

#include "GlobalContext.hpp"
#include "Render/RenderSystem.hpp"
#include "SceneGraph/Scene.h"
#include "SceneGraph/Node.h"

HierarchyPanel::HierarchyPanel()
{
    PanelName = "Scene Hierarchy";
}

void HierarchyPanel::OnUIRender()
{
    CreateTree();
}

void HierarchyPanel::DrawGameObjectNode(void* gameObject)
{
}

void HierarchyPanel::CreateTree()
{
    vkb::sg::Scene& scene = GRuntimeGlobalContext.renderSystem->GetScene();
    if (!ImGui::Begin("HierarchyPanel", &Enabled))
    {
        ImGui::End();
        return;
    }
    if (ImGui::BeginChild("##tree", ImVec2(300, 0),
                          ImGuiChildFlags_ResizeX | ImGuiChildFlags_Borders | ImGuiChildFlags_NavFlattened))
    {
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::SetNextItemShortcut(ImGuiMod_Ctrl | ImGuiKey_F, ImGuiInputFlags_Tooltip);
        ImGui::PushItemFlag(ImGuiItemFlags_NoNavDefaultFocus, true);
        if (ImGui::InputTextWithHint("##Filter", "incl,-excl", Filter.InputBuf, IM_ARRAYSIZE(Filter.InputBuf),
                                     ImGuiInputTextFlags_EscapeClearsAll))
            Filter.Build();
        ImGui::PopItemFlag();

        if (ImGui::BeginTable("##bg", 1, ImGuiTableFlags_RowBg))
        {
            for (vkb::sg::Node* node : scene.get_root_node().get_children())
                if (Filter.PassFilter(node->get_name().c_str())) // Filter root node
                    DrawTreeNode(node);
            ImGui::EndTable();
        }
    }
    ImGui::EndChild();
    ImGui::End();
}

void HierarchyPanel::DrawTreeNode(vkb::sg::Node* node)
{
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::PushID(node->get_name().c_str());
    ImGuiTreeNodeFlags tree_flags = ImGuiTreeNodeFlags_None;
    tree_flags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    // Standard opening mode as we are likely to want to add selection afterwards
    tree_flags |= ImGuiTreeNodeFlags_NavLeftJumpsBackHere; // Left arrow support
    if (node == VisibleNode)
        tree_flags |= ImGuiTreeNodeFlags_Selected;
    if (node->get_children().size() == 0)
        tree_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet;
    bool node_open = ImGui::TreeNodeEx("", tree_flags, "%s", node->get_name().c_str());
    if (ImGui::IsItemFocused())
        VisibleNode = node;
    if (node_open)
    {
        for (vkb::sg::Node* child : node->get_children())
            DrawTreeNode(child);
        ImGui::TreePop();
    }
    ImGui::PopID();
}
