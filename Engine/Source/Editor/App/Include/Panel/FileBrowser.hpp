#pragma once
#include <imgui.h>

#include "EditorInterface/Panel.hpp"
#include <filesystem>
namespace fs = std::filesystem;

class FileBrowser : public Panel
{
public:
    ~FileBrowser() override = default;
    void OnUIRender() override;

protected:
    void RenderDirectoryTreeWithSelection(const fs::path& path);

private:
};
