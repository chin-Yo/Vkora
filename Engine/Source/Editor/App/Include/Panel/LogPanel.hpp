#pragma once
#include <imgui.h>

#include "UIManage/Panel.hpp"


class LogPanel : public Panel
{
public:
    LogPanel();
    ~LogPanel() override = default;

    void Init() override;
    void OnUIRender() override;

protected:
};
