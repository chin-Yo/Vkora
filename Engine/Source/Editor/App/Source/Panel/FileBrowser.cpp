#include "Panel/FileBrowser.hpp"

#include <IconsFontAwesome5.h>
#include <imgui_internal.h>

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
    m_large_icon_font = ImGui::GetIO().Fonts->AddFontFromFileTTF(
        (Paths::GetEngineRootPath() + "/ThirdParty/imgui/fonts/fa-solid-900.ttf").c_str(),
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
    // 定义持久化的宽度状态
    static float left_pane_width = 0.0f; // 初始为0，用于后续判断是否需要初始化
    ImVec2 content_size = ImGui::GetContentRegionAvail();
    // 第一次运行时初始化宽度 (例如总宽度的 20%)
    if (left_pane_width == 0.0f)
        left_pane_width = content_size.x * 0.2f;
    // 定义分割条的宽度
    const float splitter_thickness = 4.0f;
    // 1. 左侧：Folder Tree
    // 使用 left_pane_width 变量
    ImGui::BeginChild("folder_tree", ImVec2(left_pane_width, content_size.y), true);

    ImGui::Spacing();
    is_folder_tree_hovered = false;
    constructFolderTree();

    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        if (is_folder_tree_hovered) ImGui::OpenPopup("folder_op_tree_hovered_popups");
        else ImGui::OpenPopup("folder_op_background_hovered_popups");
    }

    ImGui::EndChild(); // 结束 folder_tree

    // 弹窗构建
    constructFolderOpPopups("folder_op_background_hovered_popups");
    constructFolderOpPopups("folder_op_tree_hovered_popups", true);
    constructFolderOpPopupModal(m_selected_folder);

    // 插入 SplitterBehavior
    ImGui::SameLine(0.0f, 0.0f);

    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        ImGuiID splitter_id = window->GetID("MySplitter");

        // 2. [关键修复] 正确计算矩形坐标
        // 使用 ImVec2 直接加法，避免分量计算出错
        ImRect bb;
        bb.Min = window->DC.CursorPos; // 当前光标位置 (左上角)
        bb.Max = bb.Min; // 先把 Max 设为 Min
        bb.Max.x += splitter_thickness; // 加上宽度
        bb.Max.y += content_size.y; // 加上高度 (content_size.y 是整个区域的高度)

        // 3. 重新计算右侧剩余宽度
        // (总宽 - 左侧宽 - 分割条宽)
        float right_pane_width = content_size.x - left_pane_width - splitter_thickness;

        // 4. 调用行为逻辑
        ImGui::SplitterBehavior(bb, splitter_id, ImGuiAxis_X,
                                &left_pane_width, &right_pane_width,
                                50.0f, 50.0f,
                                4.0f);
        ImU32 col = ImGui::GetColorU32(ImGui::IsItemActive() || ImGui::IsItemHovered()
                                           ? ImGuiCol_SeparatorHovered
                                           : ImGuiCol_Separator);

        window->DrawList->AddRectFilled(bb.Min, bb.Max, col);

        // 6. [关键] 占位
        // 告诉 ImGui 这里有个东西占了位置，光标要往后移，否则右侧窗口会叠上来
        ImGui::ItemSize(bb);
    }

    // 7. 再次去掉间距，让右侧窗口紧贴分割条
    ImGui::SameLine(0.0f, 0.0f);
    // 2. 右侧：Asset Browser
    // 宽度设为 0 (自动占据剩余) 或者填入计算后的 right_pane_width
    ImGui::BeginChild("AssetBrowser", ImVec2(0, content_size.y), true);

    const uint32_t k_spacing = 4;
    ImGui::Spacing();
    ImGui::Indent((float)k_spacing);

    ImGui::BeginChild("AssetNav", ImVec2(0, 24), false);
    constructAssetNavigator();
    ImGui::EndChild();

    ImGui::Spacing();

    ImGui::BeginChild("FileListArea", ImVec2(0, 0), false);
    ImGui::Indent((float)k_spacing);
    ImGui::PushFont(DefaultFont);
    constructFolderFiles();
    ImGui::PopFont();
    ImGui::EndChild();

    // 右键菜单逻辑...
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
    ImGui::Text(GetSelectedFolderRelative().c_str());

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
                        openFolder((ui::FolderNode*)item.id);
                    }
                    else
                    {
                        // 打开文件逻辑
                    }
                }
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                {
                    if (!item.is_folder)
                    {
                        m_current_selected_item = item.name;
                        ImGui::OpenPopup("AssetPopups");
                    }
                }
            }
            constructAssetFilePopups();
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
    // 注意：不再判断点击，而是直接尝试 BeginPopup
    if (ImGui::BeginPopup("AssetPopups"))
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {2.0f, 8.0f});
        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4{0.2f, 0.2f, 0.2f, 1.0f});
        ImGui::PushFont(DefaultFont);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {8.0f, 8.0f});

        createCustomSeperatorText("COMMON");
        if (ImGui::MenuItem("  Info"))
        {
            LOG_INFO("Name : {}", m_current_selected_item)
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::MenuItem("  Delete"))
        {
            LOG_INFO("Delete");
            ImGui::CloseCurrentPopup();
        }
        if (GetSelectedFolderRelative() == "scene")
        {
            if (ImGui::MenuItem("  Open scene"))
            {
                LOG_INFO("Open scene")
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::Separator();

        createCustomSeperatorText("EXPLORE");
        if (ImGui::MenuItem("  Show in Explorer"))
        {
            LOG_INFO("Show in Explorer");
            ImGui::CloseCurrentPopup();
        }
        ImGui::Separator();

        createCustomSeperatorText("REFERENCES");
        if (ImGui::MenuItem("  Copy URL"))
        {
            LOG_INFO("Copy URL");
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::MenuItem("  Copy File Path"))
        {
            LOG_INFO("Copy file path");
            ImGui::CloseCurrentPopup();
        }

        ImGui::PopStyleVar(); // ItemSpacing
        ImGui::PopFont();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(); // WindowPadding
        ImGui::EndPopup();
    }
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
