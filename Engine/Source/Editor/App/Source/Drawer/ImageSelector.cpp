#include "Drawer/ImageSelector.hpp"

#include "GlobalContext.hpp"
#include "backends/imgui_impl_vulkan.h"
#include "Engine/Engine.hpp"
#include "Engine/Asset/AssetRegistry.hpp"
#include "Engine/Asset/Manager/AssetManager.hpp"
#include "Engine/SceneGraph/Components/Texture.hpp"
#include "Engine/Texture/Texture2D.hpp"
#include "Engine/Texture/TextureCube.hpp"
#include "Rendering/RenderSystem.hpp"
static ImGuiTextFilter textureFilter;

namespace ui
{
    bool ImageSelector::Draw(const char* label, ObserverPtr<Texture2D>& imageItem, const ImVec2& preview_size)
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

        if (open_popup)
        {
            ImGui::OpenPopup("ImageSelectorPopup");
        }

        if (ImGui::BeginPopup("ImageSelectorPopup"))
        {
            // --- 搜索栏 ---
            textureFilter.Draw("Search...", 300.0f);
            ImGui::Separator();

            // --- 滚动区域 ---
            ImGui::BeginChild("ScrollingRegion", ImVec2(0, 400), false, ImGuiWindowFlags_HorizontalScrollbar);

            // --- "None" 选项 (允许清空选择) ---
            if (textureFilter.PassFilter("None"))
            {
                if (ImGui::Selectable("None", imageItem == nullptr))
                {
                    imageItem = nullptr;
                    value_changed = true;
                    ImGui::CloseCurrentPopup();
                }
            }
            // --- 数据处理：过滤 & 分组 & 排序 ---
            // 使用 map 自动按类别名称排序 (key=类别名, value=纹理列表)
            // 注意：Texture2D* 使用原始指针仅用于UI展示，不涉及所有权转移，是安全的
            std::vector<std::pair<std::string, Texture2D*>> groupedAssets;
            auto& textureCache = GRuntimeGlobalContext.assetManager->GetTexture2DCache();
            for (const auto& [path, texturePtr] : textureCache)
            {
                if (!texturePtr) continue;

                // 过滤逻辑：匹配 文件名 或 路径
                if (textureFilter.PassFilter(texturePtr->get_name().c_str()) ||
                    textureFilter.PassFilter(path.c_str()))
                {
                    // 存入临时列表：pair<路径(用于tooltip), 纹理指针>
                    groupedAssets.push_back({path, texturePtr.get()});
                }
            }

            std::sort(groupedAssets.begin(), groupedAssets.end(),
                      [](const std::pair<std::string, Texture2D*>& a, const std::pair<std::string, Texture2D*>& b)
                      {
                          return a.second->get_name() < b.second->get_name();
                      });
            for (const auto& [assetPath, assetPtr] : groupedAssets)
            {
                ImGui::PushID(assetPtr);

                // 小图标
                ImGui::Image((ImTextureID)assetPtr->texture_id, ImVec2(20, 20));
                ImGui::SameLine();

                // 选择项
                bool isSelected = (imageItem.get() == assetPtr);
                if (ImGui::Selectable(assetPtr->get_name().c_str(), isSelected))
                {
                    // 这里我们需要从 Cache 中重新获取 shared_ptr
                    // 方法1：直接用 textureCache.at(assetPath) (最安全)
                    // 方法2：因为我们已经在循环里了，实际上并没有直接持有 shared_ptr 的引用，
                    //       所以最简单的办法是通过 key (assetPath) 去 map 里拿。
                    auto it = textureCache.find(assetPath);
                    if (it != textureCache.end())
                    {
                        imageItem = it->second; // 赋值 shared_ptr
                        value_changed = true;
                    }
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("%s", assetPath.c_str());
                }

                ImGui::PopID();
            }
            ImGui::EndChild();
            ImGui::EndPopup();
        }
        ImGui::PopID();
        return value_changed;
    }

    bool ImageSelector::Draw(const char* label, ObserverPtr<TextureCube>& imageItem, const ImVec2& preview_size)
    {
        bool value_changed = false;
        ImGui::PushID(label);

        ImGui::BeginGroup();
        ImGui::Text("%s", label);

        bool open_popup = false;

        if (imageItem)
        {
            if (ImGui::ImageButton("##preview", GRuntimeGlobalContext.assetManager->GetTexture(ICON_IMAGES)->texture_id,
                                   preview_size, ImVec2(0, 0), ImVec2(1, 1),
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

        if (open_popup)
        {
            ImGui::OpenPopup("ImageSelectorPopup");
        }

        if (ImGui::BeginPopup("ImageSelectorPopup"))
        {
            // --- 搜索栏 ---
            textureFilter.Draw("Search...", 300.0f);
            ImGui::Separator();

            // --- 滚动区域 ---
            ImGui::BeginChild("ScrollingRegion", ImVec2(0, 400), false, ImGuiWindowFlags_HorizontalScrollbar);

            // --- "None" 选项 (允许清空选择) ---
            if (textureFilter.PassFilter("None"))
            {
                if (ImGui::Selectable("None", imageItem == nullptr))
                {
                    imageItem = nullptr;
                    value_changed = true;
                    ImGui::CloseCurrentPopup();
                }
            }
            // --- 数据处理：过滤 & 分组 & 排序 ---
            // 使用 map 自动按类别名称排序 (key=类别名, value=纹理列表)
            // 注意：Texture2D* 使用原始指针仅用于UI展示，不涉及所有权转移，是安全的
            std::vector<std::pair<std::string, TextureCube*>> groupedAssets;
            auto& textureCache = GRuntimeGlobalContext.assetManager->GetTextureCubeCache();
            for (const auto& [path, texturePtr] : textureCache)
            {
                if (!texturePtr) continue;

                // 过滤逻辑：匹配 文件名 或 路径
                if (textureFilter.PassFilter(texturePtr->get_name().c_str()) ||
                    textureFilter.PassFilter(path.c_str()))
                {
                    // 存入临时列表：pair<路径(用于tooltip), 纹理指针>
                    groupedAssets.push_back({path, texturePtr.get()});
                }
            }

            std::sort(groupedAssets.begin(), groupedAssets.end(),
                      [](const std::pair<std::string, TextureCube*>& a, const std::pair<std::string, TextureCube*>& b)
                      {
                          return a.second->get_name() < b.second->get_name();
                      });
            for (const auto& [assetPath, assetPtr] : groupedAssets)
            {
                ImGui::PushID(assetPtr);

                // 小图标
                ImGui::Image(GRuntimeGlobalContext.assetManager->GetTexture(ICON_IMAGES)->texture_id, ImVec2(20, 20));
                ImGui::SameLine();

                // 选择项
                bool isSelected = (imageItem.get() == assetPtr);
                if (ImGui::Selectable(assetPtr->get_name().c_str(), isSelected))
                {
                    // 这里我们需要从 Cache 中重新获取 shared_ptr
                    // 方法1：直接用 textureCache.at(assetPath) (最安全)
                    // 方法2：因为我们已经在循环里了，实际上并没有直接持有 shared_ptr 的引用，
                    //       所以最简单的办法是通过 key (assetPath) 去 map 里拿。
                    auto it = textureCache.find(assetPath);
                    if (it != textureCache.end())
                    {
                        imageItem = it->second; // 赋值 shared_ptr
                        value_changed = true;
                    }
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("%s", assetPath.c_str());
                }

                ImGui::PopID();
            }
            ImGui::EndChild();
            ImGui::EndPopup();
        }
        ImGui::PopID();
        return value_changed;
    }
}
