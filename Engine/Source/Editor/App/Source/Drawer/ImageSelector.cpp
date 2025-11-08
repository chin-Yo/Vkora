#include "Drawer/ImageSelector.hpp"

namespace ui
{
    bool ImageSelector::Draw(const char* label, ImTextureID& current_texture_id,
                             const std::vector<ImageItem>& available_images, const ImVec2& preview_size)
    {
        bool value_changed = false;
        ImGui::PushID(label);

        ImGui::BeginGroup();
        ImGui::Text("%s", label);

        bool open_popup = false;

        if (current_texture_id != (ImTextureID)0)
        {
            if (ImGui::ImageButton("##preview", current_texture_id, preview_size, ImVec2(0, 0), ImVec2(1, 1),
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

        if (ImGui::IsItemHovered() && current_texture_id != (ImTextureID)0)
        {
            ImGui::BeginTooltip();
            ImGui::Text("Click to select another image");
            ImGui::Image(current_texture_id, ImVec2(256, 256));
            ImGui::EndTooltip();
        }

        if (open_popup)
        {
            ImGui::OpenPopup("ImageSelectorPopup");
        }

        if (ImGui::BeginPopup("ImageSelectorPopup"))
        {
            if (ImGui::Selectable("None", current_texture_id == (ImTextureID)0))
            {
                if (current_texture_id != (ImTextureID)0)
                {
                    current_texture_id = (ImTextureID)0;
                    value_changed = true;
                }
                ImGui::CloseCurrentPopup();
            }
            ImGui::Separator();

            for (const auto& item : available_images)
            {
                ImGui::Image(item.texture_id, ImVec2(24, 24));
                ImGui::SameLine();
                if (ImGui::Selectable(item.name.c_str(), current_texture_id == item.texture_id))
                {
                    if (current_texture_id != item.texture_id)
                    {
                        current_texture_id = item.texture_id;
                        value_changed = true;
                    }
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::EndPopup();
        }

        ImGui::PopID();
        return value_changed;
    }
}
