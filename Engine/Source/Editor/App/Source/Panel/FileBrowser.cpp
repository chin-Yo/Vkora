#include "Panel/FileBrowser.hpp"

#include <IconsFontAwesome5.h>

#include "Logging/Logger.hpp"
#include "Misc/Paths.hpp"
#include "UIManage/EditorGlobalContext.hpp"
#include "UIManage/EditorUIManager.hpp"


static std::filesystem::path SelectedPath;

void FileBrowser::Init()
{
    SetRootPath(Paths::GetContentPath());
    ImFontConfig config;
    static const ImWchar icon_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
    config.MergeMode = false;
    config.PixelSnapH = true;
    m_large_icon_font = ImGui::GetIO().Fonts->AddFontFromFileTTF((Paths::GetEngineRootPath() + "/ThirdParty/imgui/fonts/fa-solid-900.ttf").c_str(),
                                 64.f, &config, icon_ranges);
}

void FileBrowser::OnUIRender()
{
    if (!ImGui::Begin(ICON_FA_FOLDER " FileBrowser", &Enabled))
    {
        ImGui::End();
        return;
    }

    // 样式设置
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2.0f, 2.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 4.0f));

    const float k_folder_tree_width_scale = 0.2f;

    // 获取可用区域
    ImVec2 content_size = ImGui::GetContentRegionAvail();
    // 计算左侧宽度 (稍微减去一点以容纳分割线/间距，防止计算误差导致的换行)
    float left_pane_width = content_size.x * k_folder_tree_width_scale;

    // ---------------------------------------------------------
    // 1. 左侧：Folder Tree
    // ---------------------------------------------------------
    ImGui::BeginChild("folder_tree", ImVec2(left_pane_width, content_size.y), true);

    ImGui::Spacing();

    // 重置 hover 状态 (每一帧开始前重置)
    is_folder_tree_hovered = false;

    // 绘制树
    constructFolderTree();

    // 处理左侧区域的右键菜单
    // IsWindowHovered 检查鼠标是否在当前 Child 范围内
    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        // 如果在 constructFolderTree 内部检测到了 Item Hover，则认为是节点操作
        if (is_folder_tree_hovered)
        {
            ImGui::OpenPopup("folder_op_tree_hovered_popups");
        }
        else
        {
            ImGui::OpenPopup("folder_op_background_hovered_popups");
        }
    }

    ImGui::EndChild(); // 结束 folder_tree

    // 绘制左侧相关的弹窗 (放在 Child 之外以防被裁剪，尽管 Popup 默认是顶层的)
    constructFolderOpPopups("folder_op_background_hovered_popups");
    constructFolderOpPopups("folder_op_tree_hovered_popups", true);
    constructFolderOpPopupModal(m_selected_folder);

    ImGui::SameLine();

    // ---------------------------------------------------------
    // 2. 右侧：Asset Browser
    // ---------------------------------------------------------
    // 宽度设为 0，让 ImGui 自动计算剩余宽度，避免数学计算导致的溢出
    ImGui::BeginChild("AssetBrowser", ImVec2(0, content_size.y), true);

    const uint32_t k_spacing = 4;
    ImGui::Spacing();
    ImGui::Indent((float)k_spacing);

    // 2.1 导航栏 (高度固定 24)
    ImGui::BeginChild("AssetNav", ImVec2(0, 24), false); // border 设为 false 可能更美观
    constructAssetNavigator();
    ImGui::EndChild();

    ImGui::Spacing();

    // 2.2 文件列表区域 (占据剩余高度)
    ImGui::BeginChild("FileListArea", ImVec2(0, 0), false);
    // 注意：这里 indent 可能会导致内部布局偏移，需确认 constructFolderFiles 是否需要这个 indent
    // 如果不需要额外缩进，建议去掉 Indent，或者在 EndChild 前 Unindent
    // ImGui::Indent((float)k_spacing); 

    ImGui::PushFont(DefaultFont);
    constructFolderFiles();
    ImGui::PopFont();

    ImGui::EndChild(); // 结束 FileListArea

    // 右侧背景右键检测
    // 检测是否悬停在 AssetBrowser 窗口内，且没有悬停在任何具体 Item (如文件图标) 上
    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) &&
        !ImGui::IsAnyItemHovered() &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        ImGui::OpenPopup("folder_background_popup");
    }

    constructFolderOpPopups("folder_background_popup");
    constructFolderOpPopups("folder_dir_popup", true);
    constructFolderOpPopupModal(m_selected_file);

    ImGui::EndChild(); // 结束 AssetBrowser

    // ---------------------------------------------------------
    // 3. 记录区域信息 & 清理
    // ---------------------------------------------------------

    // 获取刚刚结束的 Item (即 AssetBrowser Child) 的矩形
    // 修正了 m_folder_rect 的赋值逻辑，假设它是 vec4(min_x, min_y, max_x, max_y)
    ImVec2 rect_min = ImGui::GetItemRectMin();
    ImVec2 rect_max = ImGui::GetItemRectMax();

    m_folder_rect.x = rect_min.x;
    m_folder_rect.y = rect_min.y;
    m_folder_rect.z = rect_max.x;
    m_folder_rect.w = rect_max.y;

    ImGui::PopStyleVar(2);
    ImGui::End(); // 结束主窗口

    // ---------------------------------------------------------
    // 4. 全局弹窗处理
    // ---------------------------------------------------------
    constructImportPopups();

    // 重置状态标志位
    is_folder_tree_hovered = false;
    is_asset_hovered = false;
}

void FileBrowser::RenderDirectoryTreeWithSelection(const std::filesystem::path& path)
{
    for (const auto& entry : std::filesystem::directory_iterator(path))
    {
        if (!entry.is_directory()) continue;

        std::string name = entry.path().filename().u8string();
        std::filesystem::path fullPath = entry.path();

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

void FileBrowser::constructAssetNavigator()
{
    ImVec2 button_size(20, 20);
    ImGui::Button(ICON_FA_ARROW_LEFT, button_size);

    ImGui::SameLine();
    ImGui::Button(ICON_FA_ARROW_RIGHT, button_size);

    ImGui::SameLine();
    static char str1[128] = "";
    ImGui::PushItemWidth(200.0f);
    ImGui::InputTextWithHint("##search_asset", (std::string(ICON_FA_SEARCH) + " Search...").c_str(), str1,
                             IM_ARRAYSIZE(str1));
    ImGui::PopItemWidth();

    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
    ImGui::Text(m_selected_folder.c_str());

    ImGui::SameLine(ImGui::GetWindowWidth() - 22);
    if (ImGui::Button(ICON_FA_COG, button_size))
    {
        ImGui::OpenPopup("asset settings");
    }

    if (ImGui::BeginPopup("asset settings"))
    {
        if (ImGui::Checkbox("show engine assets", &show_engine_assets))
        {
            pollFolders();
        }
        ImGui::EndPopup();
    }
}

void FileBrowser::constructFolderFiles()
{
    if (!m_selected_node) return;

    // --- 配置参数 ---
    float padding = 16.0f; // 单元格内边距
    float thumbnail_size = 90.0f; // 图标区域大小
    float cell_size = thumbnail_size + padding;

    float panel_width = ImGui::GetContentRegionAvail().x;
    int column_count = (int)(panel_width / cell_size); // 计算一行能放几个
    if (column_count < 1) column_count = 1;

    // 使用 ImGui 表格系统来进行网格布局 (这是最稳健的方法)
    // flags: 无边框，支持滚动
    if (ImGui::BeginTable("ContentBrowserGrid", column_count))
    {
        // 1. 合并文件夹和文件到一个列表处理，方便统一循环
        struct RenderItem
        {
            std::string name;
            bool is_folder;
            void* id;
        };
        std::vector<RenderItem> items;

        for (const auto& folder : m_selected_node->child_folders)
            items.push_back({folder->name, true, (void*)folder.get()});

        // 文件 ID 处理：因为 string 没地址，这里简单用 items.size() 做偏移
        // 实际项目中建议 FileNode 也是对象
        int file_idx = 0;
        for (const auto& file : m_selected_node->child_files)
            items.push_back({file, false, (void*)(intptr_t)((file_idx++) + 0x10000)});

        // --- 遍历渲染 ---
        for (const auto& item : items)
        {
            ImGui::TableNextColumn(); // 切换到下一个网格单元
            ImGui::PushID(item.id);

            // 获取当前图标数据
            FileIconData iconData = GetFileIconData(item.name, item.is_folder);

            // 判断是否被选中
            bool is_selected = (m_current_selected_item == item.name);

            // --- 绘制背景和交互逻辑 ---
            // 这里的 Group 是为了让整个单元格（图标+文字）作为一个整体
            ImGui::BeginGroup();

            // 计算光标位置，准备居中
            float cursor_start_x = ImGui::GetCursorPosX();

            // 1. 绘制大图标
            if (m_large_icon_font) ImGui::PushFont(m_large_icon_font);

            // 计算图标文字宽度用于居中
            float icon_width = ImGui::CalcTextSize(iconData.icon).x;
            float center_offset = (cell_size - icon_width) * 0.5f;
            if (center_offset > 0) ImGui::SetCursorPosX(cursor_start_x + center_offset);

            // 绘制带颜色的图标
            ImGui::TextColored(iconData.color, "%s", iconData.icon);

            if (m_large_icon_font) ImGui::PopFont();

            // 2. 绘制文件名
            // 恢复 X 坐标
            ImGui::SetCursorPosX(cursor_start_x);
            // 限制文字宽度，强制换行
            ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + cell_size);

            // 文字居中比较麻烦，通常直接左对齐换行，或者手动计算。
            // 这里简单做个左对齐但限制宽度的处理，视觉上还可以。
            // 如果想要严格文字居中，需要更复杂的计算。
            ImGui::TextWrapped("%s", item.name.c_str());

            ImGui::PopTextWrapPos();

            ImGui::EndGroup();


            // --- 交互处理 (InvisibleButton 覆盖法) ---
            // 获取刚才绘制的 Item 的实际矩形
            ImVec2 item_min = ImGui::GetItemRectMin();
            ImVec2 item_max = ImGui::GetItemRectMax();

            // 强制扩展点击区域到整个单元格大小，保证视觉整齐
            item_max.x = item_min.x + cell_size;
            // 高度随内容自适应，或者固定个最小值
            if (item_max.y - item_min.y < cell_size) item_max.y = item_min.y + cell_size;

            // 绘制背景 (如果选中或悬停)
            bool is_hovered = ImGui::IsItemHovered(); // 注意：这里是对 Group 的 hover

            // 因为 InvisibleButton 会挡住 Group，所以我们要先画背景，再画 Button
            // 或者先 Button 再手动画背景。这里我们使用 DrawList 在底层画背景。
            ImDrawList* draw_list = ImGui::GetWindowDrawList();

            if (is_selected)
            {
                draw_list->AddRectFilled(item_min, item_max, IM_COL32(0, 120, 215, 100), 4.0f); // 蓝色背景
                draw_list->AddRect(item_min, item_max, IM_COL32(0, 120, 215, 255), 4.0f); // 蓝色边框
            }
            else if (is_hovered)
            {
                draw_list->AddRectFilled(item_min, item_max, IM_COL32(255, 255, 255, 20), 4.0f); // 浅白悬停
            }

            // 放置全覆盖按钮捕获点击
            ImGui::SetCursorScreenPos(item_min);
            if (ImGui::InvisibleButton("##hit", ImVec2(item_max.x - item_min.x, item_max.y - item_min.y)))
            {
                // 单击：选中
                m_current_selected_item = item.name;
            }

            // 处理双击和右键 (InvisibleButton 也是 Item)
            if (ImGui::IsItemHovered())
            {
                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                {
                    if (item.is_folder)
                    {
                        // 进入文件夹逻辑
                        // EnterFolder(item.name); 
                    }
                    else
                    {
                        // 打开文件逻辑
                    }
                }
            }

            ImGui::PopID();
        }

        ImGui::EndTable();
    }
}

void FileBrowser::constructAsset(const std::string& filename, const ImVec2& size)
{
}

void FileBrowser::constructImportPopups()
{
}

void FileBrowser::constructAssetFilePopups()
{
    // right click option
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {2.0f, 8.0f});
    ImGui::PushStyleColor(ImGuiCol_PopupBg, {0.2f, 0.2f, 0.2f, 1.0f});
    ImGui::PushFont(DefaultFont);
    if (ImGui::BeginPopup("AssetPopups"))
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {8.0f, 8.0f});
        createCustomSeperatorText("COMMON");
        if (ImGui::MenuItem("  Edit"))
        {
            LOG_INFO("Edit");
        }
        if (ImGui::MenuItem("  Delete"))
        {
            LOG_INFO("Delete");
        }
        if (ImGui::MenuItem("  Export"))
        {
            LOG_INFO("Export");
        }
        ImGui::Separator();

        createCustomSeperatorText("EXPLORE");
        if (ImGui::MenuItem("  Show in Explorer"))
        {
            LOG_INFO("Show in Explorer");
        }
        ImGui::Separator();

        createCustomSeperatorText("REFERENCES");
        if (ImGui::MenuItem("  Copy URL"))
        {
            LOG_INFO("Copy URL");
        }
        if (ImGui::MenuItem("  Copy File Path"))
        {
            LOG_INFO("Copy file path");
        }
        ImGui::PopStyleVar();
        ImGui::EndPopup();
    }
    ImGui::PopFont();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}

void FileBrowser::constructFolderOpPopups(const std::string& str_id, bool is_background_not_hoverd)
{
    bool is_delete_folder = false;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {2.0f, 8.0f});
    ImGui::PushStyleColor(ImGuiCol_PopupBg, {0.2f, 0.2f, 0.2f, 1.0f});
    if (ImGui::BeginPopup(str_id.c_str()))
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {8.0f, 8.0f});
        createCustomSeperatorText("FOLDER");
        if (ImGui::MenuItem("  New Folder"))
        {
            std::string new_folder = createFolder();
            is_renaming = true;
            m_selected_file = new_folder;
        }

        if (is_background_not_hoverd)
        {
            if (ImGui::MenuItem("  Delete"))
            {
                is_delete_folder = true;
            }
            if (ImGui::MenuItem("  Rename"))
            {
                is_renaming = true;
            }
        }

        ImGui::PopStyleVar();
        ImGui::EndPopup();
    }
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    if (is_delete_folder)
    {
        ImGui::OpenPopup("Delete?");
    }
}

void FileBrowser::constructFolderOpPopupModal(const std::string& path)
{
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Delete?", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
    {
        std::string text = " Do you really want to delete " + path + "? ";
        ImGui::Text(text.c_str());
        ImGui::Separator();

        float current_width = ImGui::GetWindowWidth();
        ImVec2 button_size{current_width / 2 - 3.5f, 0.0f};

        if (ImGui::Button("Yes", button_size))
        {
            deleteFolder(path);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine(current_width / 2 + 2.0f);

        if (ImGui::Button("No", button_size))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopupModal("Create?", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::EndPopup();
    }
}

void FileBrowser::createCustomSeperatorText(const std::string& text)
{
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextBorderSize, 0.0f);
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
    ImGui::SeparatorText(text.c_str());
    ImGui::PopFont();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}

void FileBrowser::openFolder(const std::string& folder)
{
}

FileIconData FileBrowser::GetFileIconData(const std::string& filename, bool is_folder)
{
    if (is_folder)
    {
        // 文件夹：黄色
        return {ICON_FA_FOLDER, ImVec4(0.95f, 0.75f, 0.2f, 1.0f)};
    }

    // 获取扩展名 (简单实现)
    size_t dot_pos = filename.find_last_of('.');
    std::string ext = (dot_pos != std::string::npos) ? filename.substr(dot_pos) : "";

    // 转换为小写比较
    for (auto& c : ext) c = tolower(c);

    // 根据扩展名返回不同图标和颜色
    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga")
        return {ICON_FA_FILE_IMAGE, ImVec4(0.8f, 0.4f, 0.8f, 1.0f)}; // 紫色图片

    if (ext == ".cpp" || ext == ".h" || ext == ".cs" || ext == ".py" || ext == ".lua")
        return {ICON_FA_FILE_CODE, ImVec4(0.2f, 0.6f, 0.9f, 1.0f)}; // 蓝色代码

    if (ext == ".txt" || ext == ".md" || ext == ".json" || ext == ".xml")
        return {ICON_FA_FILE_ALT, ImVec4(0.9f, 0.9f, 0.9f, 1.0f)}; // 白色文本

    if (ext == ".wav" || ext == ".mp3" || ext == ".ogg")
        return {ICON_FA_FILE_AUDIO, ImVec4(0.9f, 0.5f, 0.2f, 1.0f)}; // 橙色音频

    // 默认文件图标
    return {ICON_FA_FILE, ImVec4(0.7f, 0.7f, 0.7f, 1.0f)};
}
