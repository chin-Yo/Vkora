#pragma once
#include <imgui.h>

#include <filesystem>
#include <glm/vec4.hpp>

#include "Function/FolderTree.hpp"
#include "UIManage/Panel.hpp"

struct FileIconData
{
    const char* icon;
    ImVec4 color;
};

class FileBrowser : public Panel, public ui::FolderTreeUI
{
public:
    ~FileBrowser() override = default;
    void Init() override;
    void OnUIRender() override;

protected:
    void constructAssetNavigator();
    void constructFolderFiles();
    void constructAsset(const std::string& filename, const ImVec2& size);
    void constructImportPopups();
    void constructAssetFilePopups();
    void constructFolderOpPopups(const std::string& str_id, bool is_background_not_hoverd = false);
    void constructFolderOpPopupModal(const std::string& path);

    void createCustomSeperatorText(const std::string& text);


    FileIconData GetFileIconData(const std::string& filename, bool is_folder);
    uint32_t m_poll_folder_timer_handle;
    std::string m_formatted_selected_folder;
    std::string m_selected_file;
    std::vector<std::string> m_selected_files;
    glm::vec4 m_folder_rect;
    bool is_asset_hovered = false;

    std::map<std::string, ui::HoverState> m_selected_file_hover_states;
    std::vector<std::string> m_imported_files;
    std::string m_current_selected_item;
    ImFont* m_large_icon_font;
};
