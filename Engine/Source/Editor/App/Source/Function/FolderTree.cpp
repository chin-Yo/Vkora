#include "Function/FolderTree.hpp"

#include <functional>

#include "Logging/Logger.hpp"
#include "Misc/Files.hpp"

namespace ui
{
    void FolderTreeUI::SetRootPath(const std::string& AssetRootPath_absolute)
    {
        m_root_path = AssetRootPath_absolute;
        if (!Files::Exists(m_root_path.generic_string()))
            LOG_CRITICAL("Asset root path is not exist")
        m_selected_folder = "";
        m_root_node.reset();
        pollFolders();
    }

    void FolderTreeUI::Refresh()
    {
        pollFolders();
    }

    std::string FolderTreeUI::GetSelectedFolderRelative() const
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

    void FolderTreeUI::pollFolders()
    {
        namespace fs = std::filesystem;

        // 1. 检查路径有效性
        if (m_root_path.empty() || !fs::exists(m_root_path))
            return;

        // 2. 初始化根节点 (分配内存)
        m_root_node = std::make_shared<FolderNode>();

        // 处理根路径名称 (例如 "C:/" 的 filename 可能为空，需特殊处理)
        std::string root_name = m_root_path.filename().generic_string();
        if (root_name.empty()) root_name = m_root_path.generic_string();

        m_root_node->name = root_name;
        m_root_node->setDir(m_root_path.generic_string());
        m_root_node->is_root = true;
        // 根节点没有父节点，parent_folder 默认就是空的 weak_ptr，无需操作

        // 3. 定义递归构建函数 (Lambda)
        // 参数: current_node (当前节点指针), current_path (当前节点的物理路径)
        std::function<void(std::shared_ptr<FolderNode>, const fs::path&)> buildTreeRecursive;

        buildTreeRecursive = [&](std::shared_ptr<FolderNode> current_node, const fs::path& current_path)
        {
            try
            {
                for (const auto& entry : fs::directory_iterator(current_path))
                {
                    // --- 处理文件 ---
                    if (entry.is_regular_file())
                    {
                        // set 会自动排序和去重
                        current_node->child_files.insert(entry.path().filename().generic_string());
                    }
                    // --- 处理文件夹 ---
                    else if (entry.is_directory())
                    {
                        auto child = std::make_shared<FolderNode>();
                        child->name = entry.path().filename().generic_string();
                        child->setDir(entry.path().generic_string());
                        child->is_root = false;

                        // 关键点：现在父节点本身就是 shared_ptr，可以直接赋值给子节点的 weak_ptr
                        child->parent_folder = current_node;

                        // 递归：深入构建子目录树
                        buildTreeRecursive(child, entry.path());

                        // 判断是否为叶子节点 (无子文件且无子文件夹)
                        child->is_leaf = (child->child_folders.empty());

                        // 将构建好的子节点加入 set (根据 FolderNodePtrCompare 自动排序)
                        current_node->child_folders.insert(child);
                    }
                }
            }
            catch (const std::exception& e)
            {
                // 遇到权限不足等文件夹，跳过并打印日志，不中断整个树的构建
                std::cerr << "Error reading directory " << current_path << ": " << e.what() << std::endl;
            }
        };

        // 4. 开始递归构建
        buildTreeRecursive(m_root_node, m_root_path);

        // 5. 更新根节点自身的叶子状态
        m_root_node->is_leaf = (m_root_node->child_files.empty() && m_root_node->child_folders.empty());

        // 6. 维护 UI 展开状态 Map
        // 如果 Map 中没有根目录的记录，默认将其设为展开
        if (m_folder_opened_map.find(m_root_node->dir) == m_folder_opened_map.end())
        {
            m_folder_opened_map[m_root_node->dir] = true;
        }
    }

    void FolderTreeUI::constructFolderTree()
    {
        // 检查根节点是否存在
        if (m_root_node)
        {
            constructFolderTreeRecursive(m_root_node);
        }
    }

    void FolderTreeUI::constructFolderTreeRecursive(std::shared_ptr<FolderNode> node)
    {
        if (!node) return;

        bool is_open = false; // 用于记录节点是否展开（叶子节点永远为 false）
        bool is_selected = (m_selected_folder == node->dir);

        // ---------------------------------------------------------
        // 1. 渲染逻辑分流
        // ---------------------------------------------------------
        if (node->is_leaf)
        {
            ImGui::Selectable(("   " ICON_FA_FOLDER " " + node->name).c_str(), is_selected);
        }
        else
        {
            // === 分支 B: 文件夹节点 (使用 TreeNode) ===

            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
                ImGuiTreeNodeFlags_OpenOnDoubleClick |
                ImGuiTreeNodeFlags_SpanAvailWidth;

            if (is_selected)
                flags |= ImGuiTreeNodeFlags_Selected;

            // 同步展开状态
            if (node->is_root)
                ImGui::SetNextItemOpen(true, ImGuiCond_Once);
            else
                ImGui::SetNextItemOpen(m_folder_opened_map[node->dir], ImGuiCond_Always);

            // 渲染 TreeNode
            // ID 使用指针地址
            is_open = ImGui::TreeNodeEx((void*)node.get(), flags, "%s %s",
                                        m_folder_opened_map[node->dir] ? ICON_FA_FOLDER_OPEN : ICON_FA_FOLDER,
                                        node->name.c_str());

            // 只有非叶子节点需要更新展开状态 map
            m_folder_opened_map[node->dir] = is_open;
        }

        // ---------------------------------------------------------
        // 2. 交互逻辑 (点击与选择) - 共享
        // ---------------------------------------------------------
        // 无论是 BulletText 还是 TreeNodeEx，都是 ImGui 的 Item，
        // 所以 IsItemClicked 对两者都有效。
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left) || ImGui::IsItemClicked(ImGuiMouseButton_Right))
        {
            m_selected_folder = node->dir;
            m_selected_node = node.get();
        }

        // ---------------------------------------------------------
        // 3. 右键菜单 - 共享
        // ---------------------------------------------------------
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Create Folder"))
            {
                createFolder();
            }
            if (ImGui::MenuItem("Delete"))
            {
                std::string path_to_delete = node->dir;
                ImGui::EndPopup();

                // 如果是展开的文件夹被删除了，需要 Pop
                if (is_open) ImGui::TreePop();

                deleteFolder(path_to_delete);
                return;
            }
            ImGui::EndPopup();
        }

        // ---------------------------------------------------------
        // 4. 递归渲染子节点 (仅针对展开的文件夹)
        // ---------------------------------------------------------
        if (is_open)
        {
            if (!node->child_folders.empty())
            {
                for (const auto& child : node->child_folders)
                {
                    constructFolderTreeRecursive(child);
                }
            }
            ImGui::TreePop();
        }
    }

    void FolderTreeUI::openFolder(FolderNode* new_selected_node)
    {
        m_selected_node = new_selected_node;
        m_selected_folder = new_selected_node->dir;
        if (auto lock = new_selected_node->parent_folder.lock())
        {
            m_folder_opened_map[lock->dir] = true;
        }
    }

    std::string FolderTreeUI::createFolder()
    {
        if (m_selected_folder.empty()) return "";

        std::filesystem::path parent = m_selected_folder;
        std::filesystem::path new_folder = parent / "NewFolder";

        int index = 1;
        while (std::filesystem::exists(new_folder))
        {
            new_folder = parent / ("NewFolder_" + std::to_string(index++));
        }

        try
        {
            std::filesystem::create_directory(new_folder);
            std::cout << "Created: " << new_folder << std::endl;
            Refresh();
        }
        catch (const std::exception& e)
        {
            std::cerr << "Failed to create folder: " << e.what() << std::endl;
        }

        return new_folder.generic_string();
    }

    bool FolderTreeUI::deleteFolder(const std::string& folder_path)
    {
        // 防止删除根目录
        if (folder_path == m_root_path.generic_string()) return false;

        try
        {
            // remove_all 相当于 rm -rf
            std::filesystem::remove_all(folder_path);
            std::cout << "Deleted: " << folder_path << std::endl;

            // 如果删除的是当前选中的文件夹，重置选中状态
            if (m_selected_folder == folder_path)
            {
                m_selected_folder = m_root_path.generic_string();
            }

            Refresh();
            return true;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Failed to delete folder: " << e.what() << std::endl;
            return false;
        }
    }

    bool FolderTreeUI::renderRenameInput(const std::string& full_path, const ImVec2& size)
    {
        // 简单的重命名逻辑实现示例
        // 注意：这通常需要在 constructTree 循环内部调用，用一个 bool 标记某个节点是否处于 renaming 状态

        std::filesystem::path p(full_path);
        std::string basename = p.filename().string();

        // 如果是新开始重命名，初始化 buffer
        if (m_renaming_file_path != full_path)
        {
            m_renaming_file_path = full_path;
            strncpy(new_name_buffer, basename.c_str(), sizeof(new_name_buffer) - 1);
        }

        ImGui::PushItemWidth(size.x > 0 ? size.x : 100.0f);
        ImGui::SetKeyboardFocusHere();

        bool enter_pressed = ImGui::InputText("##Rename", new_name_buffer, sizeof(new_name_buffer),
                                              ImGuiInputTextFlags_EnterReturnsTrue |
                                              ImGuiInputTextFlags_AutoSelectAll);

        ImGui::PopItemWidth();

        if (enter_pressed || ImGui::IsItemDeactivated())
        {
            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                // 执行重命名
                std::string new_name = new_name_buffer;
                if (!new_name.empty() && new_name != basename)
                {
                    std::filesystem::path new_path = p.parent_path() / new_name;
                    try
                    {
                        std::filesystem::rename(p, new_path);
                        Refresh();
                    }
                    catch (const std::exception& e)
                    {
                        std::cerr << "Rename failed: " << e.what() << std::endl;
                    }
                }
            }
            // 结束重命名
            m_renaming_file_path = "";
            return false;
        }
        return true;
    }
}
