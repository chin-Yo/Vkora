#include "Panel/FileBrowser.hpp"

#include "Misc/Paths.hpp"


static fs::path SelectedPath;

void FileBrowser::OnUIRender()
{
    if (!ImGui::Begin("FileBrowser", &Enabled))
    {
        return;
    }
    if (ImGui::BeginChild("##tree", ImVec2(300, 0),
                          ImGuiChildFlags_ResizeX | ImGuiChildFlags_Borders | ImGuiChildFlags_NavFlattened))
    {
        RenderDirectoryTreeWithSelection(fs::path(Paths::GetContentPath()));
        ImGui::EndChild();
    }
    ImGui::End();
}

void FileBrowser::RenderDirectoryTreeWithSelection(const fs::path& path)
{
    for (const auto& entry : fs::directory_iterator(path))
    {
        if (!entry.is_directory()) continue;

        std::string name = entry.path().filename().u8string();
        fs::path fullPath = entry.path();

        ImGui::PushID(fullPath.u8string().c_str());

        if (ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth))
        {
            RenderDirectoryTreeWithSelection(fullPath);
            ImGui::TreePop();
        }

        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
        {
            SelectedPath = fullPath;
        }

        if (fullPath == SelectedPath)
        {
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            ImVec2 min = ImGui::GetItemRectMin();
            ImVec2 max = ImGui::GetItemRectMax();
            drawList->AddRectFilled(min, max, IM_COL32(60, 130, 230, 100), 2.0f);
        }
        ImGui::PopID();
    }
}
