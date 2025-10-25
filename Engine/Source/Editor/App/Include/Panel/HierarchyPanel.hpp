#pragma once
#include <imgui.h>

#include "EditorInterface/Panel.hpp"


namespace scene
{
    class Node;
}

class HierarchyPanel : public Panel
{
public:
    HierarchyPanel();

    virtual void OnUIRender() override;

private:
    void DrawGameObjectNode(void* gameObject);

    ImGuiTextFilter Filter;

    scene::Node* VisibleNode = nullptr; // Selected node

    //{ Rename the node
    scene::Node* RenamingNode = nullptr;
    char RenameBuffer[256] = {0};
    //}

    scene::Node* ContextMenuNode = nullptr; // Node for context menu

    bool ExpandAllRequested = false; // Expanding all nodes in one frame will not affect the subsequent frames.
    bool CollapseAllRequested = false;


    void CreateTree();

    void DrawTreeNode(scene::Node* node);
};
