#pragma once
#include <imgui.h>

#include "EditorInterface/Panel.hpp"

class Viewport : public Panel
{
public: 
    Viewport();
    ~Viewport() override = default;

    void OnUIRender() override;

};