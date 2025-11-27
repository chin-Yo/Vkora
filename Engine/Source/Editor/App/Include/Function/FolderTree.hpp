#pragma once

#include <string>
#include <vector>
#include <queue>
#include <map>
#include <filesystem>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <set>

#include "imgui.h"
#include "IconsFontAwesome5.h"

#define DefaultFont ImGui::GetIO().Fonts->Fonts[0]

namespace ui
{
    struct FolderNode
    {
        class FolderNodePtrCompare
        {
        public:
            bool operator()(const std::shared_ptr<FolderNode>& lhs,
                            const std::shared_ptr<FolderNode>& rhs) const
            {
                return lhs->name < rhs->name;
            }
        };

        std::string name;
        std::string dir;

        std::set<std::string> child_files;
        std::set<std::shared_ptr<FolderNode>, FolderNodePtrCompare> child_folders;
        std::weak_ptr<FolderNode> parent_folder;

        bool is_root = false;
        bool is_leaf = false;

        void setDir(const std::string& dir)
        {
            this->dir = dir;
        }
    };

    struct HoverState
    {
        bool is_hovered;
        ImVec2 rect_min;
        ImVec2 rect_max;
    };

    class FolderTreeUI
    {
    public:
        FolderTreeUI() = default;
        virtual ~FolderTreeUI() = default;

        // 初始化时必须设置根目录
        void SetRootPath(const std::string& AssetRootPath_absolute);

        void draw()
        {
            // 每帧检测（也可以做成手动触发刷新）
            // pollFolders(); // 建议不要每帧调用，仅在文件变动时调用

            constructFolderTree();
        }

        // 外部调用以刷新文件列表
        void refresh()
        {
            pollFolders();
        }

        // 获取当前选中的相对路径
        std::string getSelectedFolderRelative() const
        {
            if (m_selected_folder.empty() || m_root_path.empty()) return "";
            try
            {
                return std::filesystem::relative(m_selected_folder, m_root_path).generic_string();
            }
            catch (...)
            {
                return "";
            }
        }

    protected:
        std::filesystem::path m_root_path;
        std::string m_selected_folder;
        FolderNode* m_selected_node = nullptr;
        std::shared_ptr<FolderNode> m_root_node;

        bool is_folder_tree_hovered = false;

        // 重命名缓冲区
        char new_name_buffer[256] = "";
        std::string m_renaming_file_path = ""; // 当前正在重命名的文件路径
        bool is_renaming = false;
        bool show_engine_assets = false;

        // 使用 map 存储全路径的展开状态，解决同名文件夹冲突问题
        std::map<std::string, bool> m_folder_opened_map;

        void pollFolders();
        void constructFolderTree();
        void constructFolderTreeRecursive(std::shared_ptr<FolderNode> node);

        virtual void openFolder(const std::string& folder_path);
        std::string createFolder();
        bool deleteFolder(const std::string& folder_path);

        // 返回 false 表示重命名完成（或取消），返回 true 表示正在进行
        bool renderRenameInput(const std::string& full_path, const ImVec2& size = {0.0f, 0.0f});
    };
}
