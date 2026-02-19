#pragma once
#include <optional>

#include "rttr/type.h"
#include "UIManage/Panel.hpp"


namespace scene
{
    class Scene;
}

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

    // 
    void ShowDuplicateComponentError(const rttr::type& type);
    void DrawDuplicateComponentModal();
    std::optional<rttr::type> PendingDuplicateComponentType;
    //

    scene::Node* LastSelectedNode = nullptr;
    char NodeNameBuffer[256] = {0};
};
