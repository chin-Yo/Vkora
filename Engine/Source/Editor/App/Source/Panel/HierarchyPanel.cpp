#include "Panel/HierarchyPanel.hpp"

#include <imgui.h>

#include "GlobalContext.hpp"
#include "Render/RenderSystem.hpp"
#include "Engine/SceneGraph/Scene.hpp"
#include "Engine/SceneGraph/Node.hpp"
#include "UIManage/EditorGlobalContext.hpp"
#include "World/WorldManager.hpp"

HierarchyPanel::HierarchyPanel()
{
    PanelName = "Scene Hierarchy";
}

void HierarchyPanel::OnUIRender()
{
    CreateTree();
    if (VisibleNode)
    {
        GEditorGlobalContext.selectedNode = VisibleNode;
    }
}

void HierarchyPanel::DrawGameObjectNode(void* gameObject)
{
}

void HierarchyPanel::CreateTree()
{
    scene::Scene* scene = GRuntimeGlobalContext.worldManager->GetActiveWorld();
    if (!scene)
        return;

    if (!ImGui::Begin("HierarchyPanel", &Enabled))
    {
        return;
    }
    /*if (ImGui::BeginChild("##tree", ImVec2(300, 0),
                          ImGuiChildFlags_ResizeX | ImGuiChildFlags_Borders | ImGuiChildFlags_NavFlattened))
    {*/
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::SetNextItemShortcut(ImGuiMod_Ctrl | ImGuiKey_F, ImGuiInputFlags_Tooltip);
    ImGui::PushItemFlag(ImGuiItemFlags_NoNavDefaultFocus, true);
    if (ImGui::InputTextWithHint("##Filter", "incl,-excl", Filter.InputBuf, IM_ARRAYSIZE(Filter.InputBuf),
                                 ImGuiInputTextFlags_EscapeClearsAll))
        Filter.Build();
    ImGui::PopItemFlag();

    if (ImGui::BeginTable("##bg", 1, ImGuiTableFlags_RowBg))
    {
        for (auto& node : scene->GetNodes())
        {
            if (Filter.PassFilter(node->GetName().c_str())) // Filter root node
                DrawTreeNode(node.get());
        }
        ImGui::EndTable();
    }

    ExpandAllRequested = false;
    CollapseAllRequested = false;

    if ((ContextMenuNode == nullptr) && ImGui::BeginPopupContextWindow("HierarchyContext"))
    {
        if (ImGui::MenuItem("CreateNode"))
        {
            auto* scene = GRuntimeGlobalContext.worldManager->GetActiveWorld();
            std::unique_ptr<scene::Node> node = std::make_unique<scene::Node>(scene, "Node");
            scene->AddNode(std::move(node));
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Expand All"))
        {
            ExpandAllRequested = true;
        }
        if (ImGui::MenuItem("Collapse All"))
        {
            CollapseAllRequested = true;
        }
        ImGui::EndPopup();
    }

    ContextMenuNode = nullptr;
    ImGui::End();
}

void HierarchyPanel::DrawTreeNode(scene::Node* node)
{
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGuiTreeNodeFlags tree_flags = ImGuiTreeNodeFlags_None;
    tree_flags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    // Standard opening mode as we are likely to want to add selection afterwards
    tree_flags |= ImGuiTreeNodeFlags_NavLeftJumpsBackHere; // Left arrow support
    if (node == VisibleNode)
        tree_flags |= ImGuiTreeNodeFlags_Selected;
    if (node->GetChildren().size() == 0)
        tree_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet;

    if (RenamingNode == node)
    {
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::InputText("##Rename", RenameBuffer, IM_ARRAYSIZE(RenameBuffer),
                             ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
        {
            // Submit to rename
            if (strlen(RenameBuffer) > 0)
                node->SetName(RenameBuffer);
            RenamingNode = nullptr;
        }
        else if (!ImGui::IsItemActive()) // Losing focus (such as clicking elsewhere)
        {
            // You can also choose to submit or cancel. Here, we choose to submit.
            if (strlen(RenameBuffer) > 0)
                node->SetName(RenameBuffer);
            RenamingNode = nullptr;
        }
        return;
    }
    ImGui::PushID(node->GetID());
    if (ExpandAllRequested)
    {
        ImGui::SetNextItemOpen(true);
    }
    else if (CollapseAllRequested)
    {
        ImGui::SetNextItemOpen(false);
    }
    bool node_open = ImGui::TreeNodeEx("", tree_flags, "%s", node->GetName().c_str());

    // Left-click selection
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
    {
        VisibleNode = node;
    }
    // Double-click to rename
    else if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && ImGui::IsItemHovered())
    {
        VisibleNode = node;
        RenamingNode = node;
        strncpy_s(RenameBuffer, node->GetName().c_str(), _TRUNCATE);
        ImGui::SetKeyboardFocusHere(-1); // 聚焦到下一个 widget（即 InputText）
    }
    // 在节点上右键菜单
    if (ImGui::BeginPopupContextItem())
    {
        ContextMenuNode = node;
        if (ImGui::MenuItem("Create Child"))
        {
            ContextMenuNode->CreateChild("Node");
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Delete"))
        {
            // TODO: 安全删除（延迟到帧结束或使用命令队列）
        }
        ImGui::EndPopup();
    }
    if (node_open)
    {
        for (auto& child : node->GetChildren())
            DrawTreeNode(child.get());
        ImGui::TreePop();
    }
    ImGui::PopID();
}
