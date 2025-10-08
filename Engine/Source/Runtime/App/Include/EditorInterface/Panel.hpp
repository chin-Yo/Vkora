#pragma once
#include <string>


class Panel
{
public:
    virtual ~Panel() = default;
    
    virtual void OnUIRender() = 0;
    
    void SetEnabled(bool enabled) { Enabled = enabled; }
    bool IsEnabled() const { return Enabled; }
    const std::string &GetName() const { return PanelName; }

protected:
    std::string PanelName;
    bool Enabled = true;
};
