#pragma once
#include <imgui.h>

#include "EditorInterface/Panel.hpp"
#include "SceneGraph/Scripts/Animation.h"


class HierarchyPanel : public Panel
{
public:
    HierarchyPanel();

    virtual void OnUIRender() override;

private:
    void DrawGameObjectNode(void* gameObject);

    ImGuiTextFilter Filter;
    vkb::sg::Node* VisibleNode = nullptr;
    void CreateTree();

    void DrawTreeNode(vkb::sg::Node* node);
};
