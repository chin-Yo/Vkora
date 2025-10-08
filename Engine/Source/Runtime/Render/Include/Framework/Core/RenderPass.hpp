/* Copyright (c) 2019-2025, Arm Limited and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 the "License";
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "Framework/Common/VkHelpers.hpp"
#include <volk.h>
#include <vector>
#include <optional>
#include "VulkanResource.hpp"


namespace vks
{
    class RenderPass
    {
    public:
        RenderPass(VkDevice device, VkRenderPass renderPass);
        ~RenderPass();

        RenderPass(const RenderPass&) = delete;
        RenderPass& operator=(const RenderPass&) = delete;
        RenderPass(RenderPass&& other) noexcept;
        RenderPass& operator=(RenderPass&& other) noexcept;

        VkRenderPass get() const { return m_renderPass; }
        operator VkRenderPass() const { return m_renderPass; }

    private:
        VkDevice m_device = VK_NULL_HANDLE;
        VkRenderPass m_renderPass = VK_NULL_HANDLE;
    };

    class RenderPassBuilder
    {
    public:
        RenderPassBuilder(VkDevice device);

        RenderPassBuilder& addAttachment(
            VkFormat format,
            VkSampleCountFlagBits samples,
            VkAttachmentLoadOp loadOp,
            VkAttachmentStoreOp storeOp,
            VkImageLayout initialLayout,
            VkImageLayout finalLayout);

        RenderPassBuilder& addSubpass(
            VkPipelineBindPoint bindPoint,
            const std::vector<uint32_t>& colorAttachmentIndices,
            std::optional<uint32_t> depthAttachmentIndex = std::nullopt);

        RenderPassBuilder& addDependency(
            uint32_t srcSubpass,
            uint32_t dstSubpass,
            VkPipelineStageFlags srcStageMask,
            VkPipelineStageFlags dstStageMask,
            VkAccessFlags srcAccessMask,
            VkAccessFlags dstAccessMask,
            VkDependencyFlags dependencyFlags = 0);

        /**
         * Return to the vks wrapper class, and the compiler will perform a "move constructor" operation.
         */
        RenderPass build();

        /**
         * Return the raw pointer directly. Please handle the deallocation issue yourself.
         */
        VkRenderPass buildRaw();
        VkRenderPassCreateInfo buildCreateInfo();

    private:
        VkDevice m_device;

        std::vector<VkAttachmentDescription> m_attachments;
        std::vector<VkSubpassDescription> m_subpasses;
        std::vector<VkSubpassDependency> m_dependencies;

        std::vector<std::vector<VkAttachmentReference>> m_subpassColorAttachmentRefs;
        std::vector<std::optional<VkAttachmentReference>> m_subpassDepthAttachmentRefs;
    };
}


namespace vkb
{
    struct Attachment;

    struct SubpassInfo
    {
        std::vector<uint32_t> input_attachments;

        std::vector<uint32_t> output_attachments;

        std::vector<uint32_t> color_resolve_attachments;

        bool disable_depth_stencil_attachment;

        uint32_t depth_stencil_resolve_attachment;

        VkResolveModeFlagBits depth_stencil_resolve_mode;

        std::string debug_name;
    };

    class RenderPass : public VulkanResource<VkRenderPass>
    {
    public:
        RenderPass(VulkanDevice& device,
                   const std::vector<Attachment>& attachments,
                   const std::vector<LoadStoreInfo>& load_store_infos,
                   const std::vector<SubpassInfo>& subpasses);

        RenderPass(VulkanDevice& device, VkRenderPass renderPass);

        RenderPass(const RenderPass&) = delete;

        RenderPass(RenderPass&& other);

        ~RenderPass() override;

        RenderPass& operator=(const RenderPass&) = delete;

        RenderPass& operator=(RenderPass&&) = delete;

        const uint32_t get_color_output_count(uint32_t subpass_index) const;

        VkExtent2D get_render_area_granularity() const;

    private:
        size_t subpass_count;

        template <typename T_SubpassDescription, typename T_AttachmentDescription, typename T_AttachmentReference,
                  typename T_SubpassDependency, typename T_RenderPassCreateInfo>
        void create_renderpass(const std::vector<Attachment>& attachments,
                               const std::vector<LoadStoreInfo>& load_store_infos,
                               const std::vector<SubpassInfo>& subpasses);

        std::vector<uint32_t> color_output_count;
    };
} // namespace vkb
