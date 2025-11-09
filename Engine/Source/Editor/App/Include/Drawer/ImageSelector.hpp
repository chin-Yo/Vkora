#pragma once

#include <imgui.h>
#include <string>
#include <vector>

namespace scene
{
    class Texture;
}

namespace ui
{
    struct ImageItem
    {
        std::string name; // 图像在列表中显示的名字
        ImTextureID texture_id; // ImGui的纹理ID (在Vulkan中通常是 VkDescriptorSet)
    };

    class ImageSelector
    {
    public:
        /**
        * @brief 创建一个图像/纹理选择控件。
        *
        * @param label 控件的唯一标签。
        * @param current_texture_id IN/OUT: 当前选中的纹理ID的引用。控件会读取并可能修改它。
        *                           传入 nullptr 或 (void*)0 表示 "未选择"。
        * @param available_images 包含所有可选图像信息的向量。
        * @param preview_size 预览图像框的大小。
        * @return 如果选择发生了改变，则返回 true，否则返回 false。
        */
        static bool Draw(const char* label, ImTextureID& current_texture_id,
                         const std::vector<ImageItem>& available_images,
                         const ImVec2& preview_size = ImVec2(64, 64));

        static ImTextureID GetTextureID(scene::Texture& texture);
    };
}
