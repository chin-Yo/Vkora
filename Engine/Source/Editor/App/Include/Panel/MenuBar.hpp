#pragma once
#include <imgui.h>

#include "EditorInterface/Panel.hpp"

class MenuBar : public Panel
{
public: 
    MenuBar();
    ~MenuBar() override = default;

    void OnUIRender() override;

};