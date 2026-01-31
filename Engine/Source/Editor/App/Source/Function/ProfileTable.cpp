#include "Function/ProfileTable.hpp"


void ProfileDrawer::RenderProfileNode(const ProfileResult& node, double totalFrameTime)
{
    // 重要：为了防止同名节点导致 ImGui ID 冲突（比如循环中多次调用的同名函数），
    // 我们使用节点的内存地址作为 ID 种子。
    ImGui::PushID(&node);

    ImGui::TableNextRow();

    // --- Column 1: 名称 (Tree) ---
    ImGui::TableNextColumn();

    // 标记设置：如果没有子节点，显示为 Bullet（圆点）；否则显示箭头
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanFullWidth;
    if (node.children.empty())
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    else
        flags |= ImGuiTreeNodeFlags_OpenOnArrow;

    // 默认展开耗时超过 20% 的节点，方便快速定位瓶颈
    if (totalFrameTime > 0.0 && (node.durationMs / totalFrameTime) > 0.2)
        flags |= ImGuiTreeNodeFlags_DefaultOpen;

    bool isOpen = ImGui::TreeNodeEx(node.name, flags);

    // --- Column 2: 耗时 (Text) ---
    ImGui::TableNextColumn();
    // 渲染文字，数字右对齐通常更易读，但 Table 列通常会自动左对齐。
    // 如果想要严格对其小数点，可以手动处理，这里保持简单。
    if (node.durationMs < 0.001)
        ImGui::Text("< 0.001 ms");
    else
        ImGui::Text("%.3f ms", node.durationMs);

    // --- Column 3: 占比 (Progress Bar) ---
    ImGui::TableNextColumn();

    float fraction = (totalFrameTime > 0.0) ? (float)(node.durationMs / totalFrameTime) : 0.0f;

    // 动态颜色：红色(高负载) -> 黄色 -> 绿色(低负载)
    ImVec4 color;
    if (fraction > 0.5f) color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f); // >50% 红
    else if (fraction > 0.1f) color = ImVec4(1.0f, 0.8f, 0.3f, 1.0f); // >10% 黄
    else color = ImVec4(0.5f, 0.8f, 0.5f, 1.0f); // <10% 绿

    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, color);

    // 进度条叠加文字
    char overlay[32];
    sprintf(overlay, "%.1f%%", fraction * 100.0f);

    // 获取当前列宽
    float width = ImGui::GetContentRegionAvail().x;
    ImGui::ProgressBar(fraction, ImVec2(width, 0.0f), overlay);

    ImGui::PopStyleColor();

    // --- 递归处理子节点 ---
    if (isOpen && !node.children.empty())
    {
        for (const auto& child : node.children)
        {
            RenderProfileNode(child, totalFrameTime);
        }
        ImGui::TreePop();
    }

    ImGui::PopID(); // 配对 PushID
}

void ProfileDrawer::ShowProfilerWindow(const std::vector<ProfileResult>& rootNodes, bool* p_open)
{
    if (ImGui::Begin("Performance Profiler", p_open))
    {
        // 1. 计算总耗时 (Sum of Roots)
        double totalFrameTime = 0.0;
        for (const auto& node : rootNodes)
            totalFrameTime += node.durationMs;

        // 顶部显示总览信息
        ImGui::Text("Total Frame Time: %.3f ms (%.0f FPS)", totalFrameTime,
                    (totalFrameTime > 0 ? 1000.0 / totalFrameTime : 0));
        ImGui::Separator();

        // 2. 开启表格
        // Flags说明:
        // BordersOuterH/V: 外边框
        // RowBg: 隔行变色 (美观关键)
        // Resizable: 允许用户拖拽列宽
        // Reorderable: 允许用户拖拽列顺序
        ImGuiTableFlags tableFlags = ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_BordersV |
            ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable;

        if (ImGui::BeginTable("ProfilerTable", 3, tableFlags))
        {
            // 3. 设置表头
            // Name 列设为 NoHide，防止被用户意外隐藏
            ImGui::TableSetupColumn("Function Name",
                                    ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthStretch, 0.5f);
            ImGui::TableSetupColumn("Duration", ImGuiTableColumnFlags_WidthFixed, 80.0f); // 固定宽度
            ImGui::TableSetupColumn("Total %", ImGuiTableColumnFlags_WidthStretch, 0.3f);

            // 绘制表头行
            ImGui::TableHeadersRow();

            // 4. 遍历根节点数组
            for (const auto& node : rootNodes)
            {
                RenderProfileNode(node, totalFrameTime);
            }

            ImGui::EndTable();
        }
    }
    ImGui::End();
}
