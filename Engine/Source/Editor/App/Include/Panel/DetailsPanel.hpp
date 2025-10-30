#pragma once
#include "EditorInterface/Panel.hpp"


namespace scene
{
    class Transform;
    class Node;
}

class DetailsPanel : public Panel
{
public:
    DetailsPanel();
    ~DetailsPanel() override;

    void OnUIRender() override;

private:
    void DisplaySelectedNode(scene::Node* node);
    void DrawComponentSelector(scene::Node* node);
    void DrawTransformInspector(scene::Transform& transform);
};
