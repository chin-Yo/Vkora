#include "Drawer/ImageSelector.hpp"

#include "GlobalContext.hpp"
#include "backends/imgui_impl_vulkan.h"
#include "Engine/Asset/AssetRegistry.hpp"
#include "Engine/Asset/Manager/AssetManager.hpp"
#include "Engine/SceneGraph/Components/Image.hpp"
#include "Engine/SceneGraph/Components/Texture.hpp"
#include "Engine/Texture/Texture2D.hpp"
#include "Render/RenderSystem.hpp"

namespace ui
{
    bool ImageSelector::Draw(const char* label, std::shared_ptr<Texture2D>& imageItem, const ImVec2& preview_size)
    {
        bool value_changed = false;
        ImGui::PushID(label);

        ImGui::BeginGroup();
        ImGui::Text("%s", label);

        bool open_popup = false;

        if (imageItem && imageItem->texture_id != (ImTextureID)0)
        {
            if (ImGui::ImageButton("##preview", imageItem->texture_id, preview_size, ImVec2(0, 0), ImVec2(1, 1),
                                   ImVec4(0, 0, 0, 0), ImVec4(1, 1, 1, 1)))
            {
                open_popup = true;
            }
        }
        else
        {
            if (ImGui::InvisibleButton("##placeholder", preview_size))
            {
                open_popup = true;
            }
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            ImVec2 p0 = ImGui::GetItemRectMin();
            ImVec2 p1 = ImGui::GetItemRectMax();
            draw_list->AddRectFilled(p0, p1, IM_COL32(50, 50, 50, 255));
            draw_list->AddRect(p0, p1, IM_COL32(200, 200, 200, 100));
            float cross_extent = preview_size.x * 0.25f;
            ImVec2 center = ImVec2((p0.x + p1.x) * 0.5f, (p0.y + p1.y) * 0.5f);
            draw_list->AddLine(ImVec2(center.x - cross_extent, center.y), ImVec2(center.x + cross_extent, center.y),
                               IM_COL32(200, 200, 200, 150), 1.0f);
            draw_list->AddLine(ImVec2(center.x, center.y - cross_extent), ImVec2(center.x, center.y + cross_extent),
                               IM_COL32(200, 200, 200, 150), 1.0f);
        }
        ImGui::EndGroup();

        /*if (ImGui::IsItemHovered() && imageItem->texture_id != (ImTextureID)0)
        {
            ImGui::BeginTooltip();
            ImGui::Text("Click to select another image");
            ImGui::Image(imageItem->texture_id, ImVec2(256, 256));
            ImGui::EndTooltip();
        }*/

        if (open_popup)
        {
            ImGui::OpenPopup("ImageSelectorPopup");
        }

        if (ImGui::BeginPopup("ImageSelectorPopup"))
        {
            ImGui::PushItemWidth(-1); // 可选：让内容占满宽度

            // 限制最大高度并启用滚动
            float max_height = 300.0f; // 例如最大 300 像素高
            if (ImGui::BeginChild("ImageList", ImVec2(0, max_height), false,
                                  ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
            {
                if (ImGui::Selectable("None", imageItem == nullptr))
                {
                    if (!imageItem)
                    {
                        imageItem.reset();
                        value_changed = true;
                    }
                    ImGui::CloseCurrentPopup();
                }
                ImGui::Separator();

                auto& Textures = GRuntimeGlobalContext.renderSystem->GetAssetManager()->GetTextureCache();
                for (auto& item : Textures)
                {
                    if (!item.second)
                    {
                        continue;
                    }
                    ImGui::Image(item.second->texture_id, ImVec2(24, 24));
                    ImGui::SameLine();
                    if (ImGui::Selectable(item.second->get_name().c_str(),
                                          imageItem && imageItem->get_name() == item.first))
                    {
                        if (imageItem != item.second)
                        {
                            imageItem = item.second;
                            value_changed = true;
                        }
                        ImGui::CloseCurrentPopup();
                    }
                }
            }
            ImGui::EndChild();
            ImGui::PopItemWidth();

            ImGui::EndPopup();
        }

        ImGui::PopID();
        return value_changed;
    }

    ImTextureID ImageSelector::GetTextureID(scene::Texture& texture)
    {
        auto* image = texture.get_image();
        auto* sampler = texture.get_sampler();

        return reinterpret_cast<ImTextureID>(ImGui_ImplVulkan_AddTexture(sampler->vk_sampler.GetHandle(),
                                                                         image->get_vk_image_view().GetHandle(),
                                                                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
    }
}
