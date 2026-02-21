#pragma once
#include <imgui.h>

#include "UIManage/Panel.hpp"


class MenuBar : public Panel
{
public:
    MenuBar();
    ~MenuBar() override = default;

    void OnUIRender() override;

protected:
    void DrawMenuPanel();

    bool bShowStyleEditor = false;
    bool bShowProfileDrawer = false;
};
