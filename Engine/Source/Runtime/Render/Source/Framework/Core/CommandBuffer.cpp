#include "Framework/Core/CommandBuffer.hpp"
#include "Framework/Core/CommandPool.hpp"
#include "Framework/Core/QueryPool.hpp"
#include "Framework/Rendering/RenderTarget.hpp"
#include "Framework/Core/Buffer.hpp"
#include "Framework/Core/Sampler.hpp"
#include "Framework/Rendering/Subpass.hpp"
#include "Framework/Rendering/RenderFrame.hpp"
#include "Framework/Core/RenderPass.hpp"
#include "Framework/Core/VulkanDevice.hpp"

namespace vkb
{
    CommandBuffer::CommandBuffer(CommandPool& command_pool_, VkCommandBufferLevel level_)
        : VulkanResource<VkCommandBuffer>(VK_NULL_HANDLE, &command_pool_.get_device()),
          level(level_),
          command_pool(command_pool_),
          max_push_constants_size(command_pool_.get_device().get_gpu().get_properties().limits.maxPushConstantsSize)
    {
        VkCommandBufferAllocateInfo allocate_info{};
        allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocate_info.pNext = nullptr;
        allocate_info.commandPool = command_pool.get_handle();
        allocate_info.level = level;
        allocate_info.commandBufferCount = 1;

        VkCommandBuffer handle = VK_NULL_HANDLE;
        VkResult result = vkAllocateCommandBuffers(GetDevice().GetHandle(), &allocate_info, &handle);

        if (result != VK_SUCCESS)
        {
            VK_CHECK_RESULT(result);
            handle = VK_NULL_HANDLE;
        }

        SetHandle(handle);
    }

    CommandBuffer::~CommandBuffer()
    {
        if (GetHandle() != VK_NULL_HANDLE)
        {
            vkFreeCommandBuffers(GetDevice().GetHandle(), command_pool.get_handle(), 1, &GetHandle());
        }
    }

    void CommandBuffer::begin(VkCommandBufferUsageFlags flags,
                              vkb::CommandBuffer* primary_cmd_buf)
    {
        begin_impl(flags, primary_cmd_buf);
    }

    void CommandBuffer::begin_impl(VkCommandBufferUsageFlags flags, vkb::CommandBuffer* primary_cmd_buf)
    {
        if (level == VK_COMMAND_BUFFER_LEVEL_SECONDARY)
        {
            assert(
                primary_cmd_buf &&
                "A primary command buffer pointer must be provided when calling begin from a secondary one");
            return begin_impl(flags, primary_cmd_buf->current_render_pass, primary_cmd_buf->current_framebuffer,
                              primary_cmd_buf->pipeline_state.get_subpass_index());
        }
        else
        {
            return begin_impl(flags, nullptr, nullptr, 0);
        }
    }

    void CommandBuffer::begin(VkCommandBufferUsageFlags flags,
                              const vkb::RenderPass* render_pass,
                              const vkb::Framebuffer* framebuffer,
                              uint32_t subpass_index)
    {
        begin_impl(flags, render_pass, framebuffer, subpass_index);
    }

    void CommandBuffer::begin_impl(VkCommandBufferUsageFlags flags,
                                   const vkb::RenderPass* render_pass,
                                   const vkb::Framebuffer* framebuffer,
                                   uint32_t subpass_index)
    {
        pipeline_state.reset();
        resource_binding_state.reset();
        descriptor_set_layout_binding_state.clear();
        stored_push_constants.clear();

        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.pNext = nullptr;
        begin_info.flags = flags;
        begin_info.pInheritanceInfo = nullptr;

        VkCommandBufferInheritanceInfo inheritance_info{};
        inheritance_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
        inheritance_info.pNext = nullptr;

        if (level == VK_COMMAND_BUFFER_LEVEL_SECONDARY)
        {
            assert(
                (render_pass && framebuffer) &&
                "Render pass and framebuffer must be provided when calling begin from a secondary one");

            current_render_pass = render_pass;
            current_framebuffer = framebuffer;

            inheritance_info.renderPass = current_render_pass->GetHandle();
            inheritance_info.framebuffer = current_framebuffer->get_handle();
            inheritance_info.subpass = subpass_index;

            begin_info.pInheritanceInfo = &inheritance_info;
        }

        VkResult result = vkBeginCommandBuffer(this->GetHandle(), &begin_info);

        if (result != VK_SUCCESS)
        {
            LOG_ERROR("Failed to begin command buffer")
        }
    }

    void CommandBuffer::begin_query(QueryPool const& query_pool, uint32_t query, VkQueryControlFlags flags)
    {
        vkCmdBeginQuery(
            this->GetHandle(),
            query_pool.get_handle(),
            query,
            flags);
    }

    void CommandBuffer::begin_render_pass(RenderTarget const& render_target,
                                          std::vector<vkb::LoadStoreInfo> const& load_store_infos,
                                          std::vector<VkClearValue> const& clear_values,
                                          std::vector<std::unique_ptr<vkb::Subpass>> const& subpasses,
                                          VkSubpassContents contents)
    {
        // Reset state
        pipeline_state.reset();
        resource_binding_state.reset();
        descriptor_set_layout_binding_state.clear();

        auto& render_pass = get_render_pass(render_target, load_store_infos, subpasses);
        auto& framebuffer = this->GetDevice().get_resource_cache().request_framebuffer(render_target, render_pass);

        begin_render_pass(render_target, render_pass, framebuffer, clear_values, contents);
    }

    void CommandBuffer::begin_render_pass(RenderTarget const& render_target,
                                          RenderPass const& render_pass,
                                          Framebuffer const& framebuffer,
                                          std::vector<VkClearValue> const& clear_values,
                                          VkSubpassContents contents)
    {
        begin_render_pass_impl(render_target, render_pass, framebuffer, clear_values, contents);
    }

    void CommandBuffer::begin_render_pass_impl(RenderTarget const& render_target,
                                               vkb::RenderPass const& render_pass,
                                               Framebuffer const& framebuffer,
                                               std::vector<VkClearValue> const& clear_values,
                                               VkSubpassContents contents)
    {
        current_render_pass = &render_pass;
        current_framebuffer = &framebuffer;

        // Begin render pass using C API
        VkRenderPassBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        begin_info.renderPass = current_render_pass->GetHandle();
        begin_info.framebuffer = current_framebuffer->get_handle();
        begin_info.renderArea.extent = render_target.get_extent();
        begin_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
        begin_info.pClearValues = clear_values.data();

        const auto& framebuffer_extent = current_framebuffer->get_extent();

        // Test render area optimization
        if (!is_render_size_optimal(framebuffer_extent, begin_info.renderArea))
        {
            bool framebuffer_changed =
                (framebuffer_extent.width != last_framebuffer_extent.width) ||
                (framebuffer_extent.height != last_framebuffer_extent.height);

            bool render_area_changed =
                (begin_info.renderArea.extent.width != last_render_area_extent.width) ||
                (begin_info.renderArea.extent.height != last_render_area_extent.height);

            if (framebuffer_changed || render_area_changed)
            {
                // TODO LOG("Render target extent is not an optimal size");
            }
            last_framebuffer_extent = framebuffer_extent;
            last_render_area_extent = begin_info.renderArea.extent;
        }

        vkCmdBeginRenderPass(this->GetHandle(), &begin_info, contents);

        // Update blend state attachments
        auto blend_state = pipeline_state.get_color_blend_state();
        blend_state.attachments.resize(current_render_pass->get_color_output_count(pipeline_state.get_subpass_index()));
        pipeline_state.set_color_blend_state(blend_state);
    }

    void CommandBuffer::bind_buffer(
        vkb::Buffer const& buffer, DeviceSizeType offset, DeviceSizeType range, uint32_t set, uint32_t binding,
        uint32_t array_element)
    {
        resource_binding_state.bind_buffer(
            buffer,
            offset,
            range,
            set,
            binding,
            array_element);
    }

    void CommandBuffer::bind_image(
        vkb::ImageView const& image_view, vkb::Sampler const& sampler, uint32_t set, uint32_t binding,
        uint32_t array_element)
    {
        resource_binding_state.bind_image(
            image_view,
            sampler,
            set,
            binding,
            array_element);
    }

    void CommandBuffer::bind_image(vkb::ImageView const& image_view, uint32_t set, uint32_t binding,
                                   uint32_t array_element)
    {
        resource_binding_state.bind_image(
            image_view,
            set,
            binding,
            array_element);
    }

    void CommandBuffer::bind_index_buffer(vkb::Buffer const& buffer, VkDeviceSize offset, VkIndexType index_type)
    {
        vkCmdBindIndexBuffer(GetHandle(), buffer.GetHandle(), offset, index_type);
    }

    void CommandBuffer::bind_input(vkb::ImageView const& image_view, uint32_t set, uint32_t binding,
                                   uint32_t array_element)
    {
        resource_binding_state.bind_input(image_view, set, binding, array_element);
    }

    void CommandBuffer::bind_lighting(vkb::LightingState& lighting_state, uint32_t set, uint32_t binding)
    {
        bind_buffer(lighting_state.light_buffer.get_buffer(), lighting_state.light_buffer.get_offset(),
                    lighting_state.light_buffer.get_size(), set, binding, 0);

        set_specialization_constant(0, to_u32(lighting_state.directional_lights.size()));
        set_specialization_constant(1, to_u32(lighting_state.point_lights.size()));
        set_specialization_constant(2, to_u32(lighting_state.spot_lights.size()));
    }

    void CommandBuffer::bind_pipeline_layout(vkb::PipelineLayout& pipeline_layout)
    {
        pipeline_state.set_pipeline_layout(pipeline_layout);
    }

    void CommandBuffer::bind_vertex_buffers(uint32_t first_binding,
                                            std::vector<std::reference_wrapper<const vkb::Buffer>> const& buffers,
                                            std::vector<VkDeviceSize> const& offsets)
    {
        if (buffers.size() != offsets.size())
        {
            return;
        }

        std::vector<VkBuffer> buffer_handles;
        buffer_handles.reserve(buffers.size());
        std::transform(buffers.begin(), buffers.end(), std::back_inserter(buffer_handles),
                       [](auto const& buffer_wrapper)
                       {
                           return buffer_wrapper.get().GetHandle();
                       });

        vkCmdBindVertexBuffers(GetHandle(),
                               first_binding,
                               static_cast<uint32_t>(buffers.size()),
                               buffer_handles.data(),
                               offsets.data());
    }

    void CommandBuffer::blit_image(vkb::Image const& src_img, vkb::Image const& dst_img,
                                   std::vector<VkImageBlit> const& regions)
    {
        vkCmdBlitImage(GetHandle(),
                       src_img.GetHandle(),
                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       dst_img.GetHandle(),
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       static_cast<uint32_t>(regions.size()),
                       regions.data(),
                       VK_FILTER_NEAREST);
    }

    void CommandBuffer::buffer_memory_barrier(vkb::Buffer const& buffer,
                                              DeviceSizeType offset,
                                              DeviceSizeType size,
                                              BufferMemoryBarrierType const& memory_barrier)
    {
        buffer_memory_barrier_impl(buffer, offset, size, memory_barrier);
    }

    void CommandBuffer::buffer_memory_barrier_impl(vkb::Buffer const& buffer,
                                                   DeviceSizeType offset,
                                                   DeviceSizeType size,
                                                   vkb::BufferMemoryBarrier const& memory_barrier)
    {
        VkBufferMemoryBarrier buffer_memory_barrier;
        buffer_memory_barrier.srcAccessMask = memory_barrier.src_access_mask;
        buffer_memory_barrier.dstAccessMask = memory_barrier.dst_access_mask;
        buffer_memory_barrier.buffer = buffer.GetHandle();
        buffer_memory_barrier.offset = offset;
        buffer_memory_barrier.size = size;

        vkCmdPipelineBarrier(GetHandle(), memory_barrier.src_stage_mask, memory_barrier.dst_stage_mask, 0,
                             0, nullptr,
                             1, &buffer_memory_barrier,
                             0, nullptr);
    }

    void CommandBuffer::clear(ClearAttachmentType const& attachment, ClearRectType const& rect)
    {
        vkCmdClearAttachments(GetHandle(), 1, &attachment, 1, &rect);
    }

    void CommandBuffer::copy_buffer(vkb::Buffer const& src_buffer,
                                    vkb::Buffer const& dst_buffer,
                                    DeviceSizeType size)
    {
        copy_buffer_impl(src_buffer, dst_buffer, size);
    }

    void
    CommandBuffer::copy_buffer_impl(vkb::Buffer const& src_buffer, vkb::Buffer const& dst_buffer, VkDeviceSize size)
    {
        VkBufferCopy copy_region{};
        copy_region.srcOffset = 0;
        copy_region.dstOffset = 0;
        copy_region.size = size;
        vkCmdCopyBuffer(GetHandle(), src_buffer.GetHandle(), dst_buffer.GetHandle(), 1, &copy_region);
    }

    void CommandBuffer::copy_buffer_to_image(vkb::Buffer const& buffer,
                                             vkb::Image const& image,
                                             std::vector<BufferImageCopyType> const& regions)
    {
        if (regions.empty())
        {
            return;
        }
        vkCmdCopyBufferToImage(GetHandle(),
                               buffer.GetHandle(),
                               image.GetHandle(),
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               static_cast<uint32_t>(regions.size()),
                               regions.data());
    }

    void vkb::CommandBuffer::copy_image(const vkb::Image& src_img, const vkb::Image& dst_img,
                                        const std::vector<VkImageCopy>& regions)
    {
        vkCmdCopyImage(
            GetHandle(),
            src_img.GetHandle(),
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            dst_img.GetHandle(),
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            static_cast<uint32_t>(regions.size()),
            regions.data());
    }

    void vkb::CommandBuffer::copy_image_to_buffer(const vkb::Image& image,
                                                  VkImageLayout image_layout,
                                                  const vkb::Buffer& buffer,
                                                  const std::vector<VkBufferImageCopy>& regions)
    {
        vkCmdCopyImageToBuffer(
            GetHandle(),
            image.GetHandle(),
            image_layout,
            buffer.GetHandle(),
            static_cast<uint32_t>(regions.size()),
            regions.data());
    }

    void vkb::CommandBuffer::dispatch(uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z)
    {
        flush(VK_PIPELINE_BIND_POINT_COMPUTE);
        vkCmdDispatch(GetHandle(), group_count_x, group_count_y, group_count_z);
    }

    void vkb::CommandBuffer::dispatch_indirect(const vkb::Buffer& buffer, VkDeviceSize offset)
    {
        flush(VK_PIPELINE_BIND_POINT_COMPUTE);

        vkCmdDispatchIndirect(GetHandle(), buffer.GetHandle(), offset);
    }

    void CommandBuffer::draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex,
                             uint32_t first_instance)
    {
        flush(VK_PIPELINE_BIND_POINT_GRAPHICS);
        vkCmdDraw(this->GetHandle(), vertex_count, instance_count, first_vertex, first_instance);
    }

    void CommandBuffer::draw_indexed(
        uint32_t index_count, uint32_t instance_count, uint32_t first_index, int32_t vertex_offset,
        uint32_t first_instance)
    {
        flush(VK_PIPELINE_BIND_POINT_GRAPHICS);
        vkCmdDrawIndexed(this->GetHandle(), index_count, instance_count, first_index, vertex_offset, first_instance);
    }

    void CommandBuffer::draw_indexed_indirect(vkb::Buffer const& buffer, VkDeviceSize offset, uint32_t draw_count,
                                              uint32_t stride)
    {
        flush(VK_PIPELINE_BIND_POINT_GRAPHICS);
        vkCmdDrawIndexedIndirect(this->GetHandle(), buffer.GetHandle(), offset, draw_count, stride);
    }

    void CommandBuffer::end()
    {
        if (vkEndCommandBuffer(this->GetHandle()) != VK_SUCCESS)
        {
            LOG_ERROR("Failed to end command buffer")
        }
    }

    void CommandBuffer::end_query(QueryPoolType const& query_pool, uint32_t query)
    {
        // TODO: Assuming query_pool.get_handle() returns a valid VkQueryPool handle
        vkCmdEndQuery(this->GetHandle(), query_pool.get_handle(), query);
    }

    void CommandBuffer::end_render_pass()
    {
        vkCmdEndRenderPass(this->GetHandle());
    }

    VkCommandBufferLevel CommandBuffer::get_level() const
    {
        // Assuming the member variable 'level' is of type VkCommandBufferLevel
        return level;
    }

    void CommandBuffer::execute_commands(vkb::CommandBuffer& secondary_command_buffer)
    {
        // vkCmdExecuteCommands expects a pointer to an array of command buffers
        // TODO: Assuming get_resource() returns a valid VkCommandBuffer handle
        VkCommandBuffer secondary_cmd_buffer_handle = secondary_command_buffer.GetHandle();
        vkCmdExecuteCommands(this->GetHandle(), 1, &secondary_cmd_buffer_handle);
    }

    void CommandBuffer::execute_commands(std::vector<std::shared_ptr<vkb::CommandBuffer>>& secondary_command_buffers)
    {
        execute_commands_impl(secondary_command_buffers);
    }

    void CommandBuffer::execute_commands_impl(
        std::vector<std::shared_ptr<vkb::CommandBuffer>>& secondary_command_buffers)
    {
        if (secondary_command_buffers.empty())
        {
            return;
        }

        std::vector<VkCommandBuffer> sec_cmd_buf_handles;
        sec_cmd_buf_handles.reserve(secondary_command_buffers.size());

        std::transform(secondary_command_buffers.begin(),
                       secondary_command_buffers.end(),
                       std::back_inserter(sec_cmd_buf_handles),
                       [](const std::shared_ptr<vkb::CommandBuffer>& sec_cmd_buf)
                       {
                           // TODO: Assuming get_resource() returns a valid VkCommandBuffer handle
                           return sec_cmd_buf->GetHandle();
                       });

        vkCmdExecuteCommands(this->GetHandle(),
                             static_cast<uint32_t>(sec_cmd_buf_handles.size()),
                             sec_cmd_buf_handles.data());
    }

    typename vkb::CommandBuffer::RenderPassType&
    CommandBuffer::get_render_pass(RenderTargetType const& render_target,
                                   std::vector<LoadStoreInfoType> const& load_store_infos,
                                   std::vector<std::unique_ptr<vkb::Subpass>> const& subpasses)
    {
        return get_render_pass_impl(this->GetDevice(), render_target, load_store_infos, subpasses);
    }

    vkb::RenderPass&
    CommandBuffer::get_render_pass_impl(vkb::VulkanDevice& device,
                                        vkb::RenderTarget const& render_target,
                                        std::vector<vkb::LoadStoreInfo> const& load_store_infos,
                                        std::vector<std::unique_ptr<vkb::Subpass>> const& subpasses)
    {
        // Create render pass
        assert(subpasses.size() > 0 && "Cannot create a render pass without any subpass");

        std::vector<vkb::SubpassInfo> subpass_infos(subpasses.size());
        auto subpass_info_it = subpass_infos.begin();
        for (auto& subpass : subpasses)
        {
            subpass_info_it->input_attachments = subpass->get_input_attachments();
            subpass_info_it->output_attachments = subpass->get_output_attachments();
            subpass_info_it->color_resolve_attachments = subpass->get_color_resolve_attachments();
            subpass_info_it->disable_depth_stencil_attachment = subpass->get_disable_depth_stencil_attachment();
            subpass_info_it->depth_stencil_resolve_mode = subpass->get_depth_stencil_resolve_mode();
            subpass_info_it->depth_stencil_resolve_attachment = subpass->get_depth_stencil_resolve_attachment();
            subpass_info_it->debug_name = subpass->get_debug_name();

            ++subpass_info_it;
        }

        return device.get_resource_cache().request_render_pass(render_target.get_attachments(), load_store_infos,
                                                               subpass_infos);
    }

    void CommandBuffer::image_memory_barrier(RenderTargetType& render_target, uint32_t view_index,
                                             ImageMemoryBarrierType const& memory_barrier) const
    {
        auto const& image_view = render_target.get_views()[view_index];

        image_memory_barrier_impl(image_view, memory_barrier);

        render_target.set_layout(view_index, memory_barrier.new_layout);
    }

    void CommandBuffer::image_memory_barrier(ImageViewType const& image_view,
                                             ImageMemoryBarrierType const& memory_barrier) const
    {
        image_memory_barrier_impl(image_view, memory_barrier);
    }

    void CommandBuffer::image_memory_barrier_impl(vkb::ImageView const& image_view,
                                                  vkb::ImageMemoryBarrier const& memory_barrier) const
    {
        VkImageSubresourceRange subresource_range = image_view.get_subresource_range();
        VkFormat format = image_view.get_format();

        if (is_depth_only_format(format))
        {
            subresource_range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        }
        else if (is_depth_stencil_format(format))
        {
            subresource_range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        }

        VkImageMemoryBarrier image_memory_barrier{};
        image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        image_memory_barrier.srcAccessMask = memory_barrier.src_access_mask;
        image_memory_barrier.dstAccessMask = memory_barrier.dst_access_mask;
        image_memory_barrier.oldLayout = memory_barrier.old_layout;
        image_memory_barrier.newLayout = memory_barrier.new_layout;
        image_memory_barrier.srcQueueFamilyIndex = memory_barrier.src_queue_family;
        image_memory_barrier.dstQueueFamilyIndex = memory_barrier.dst_queue_family;
        image_memory_barrier.image = image_view.get_image().GetHandle();
        image_memory_barrier.subresourceRange = subresource_range;

        vkCmdPipelineBarrier(
            this->GetHandle(), // VkCommandBuffer
            memory_barrier.src_stage_mask,
            memory_barrier.dst_stage_mask,
            0, // dependencyFlags
            0, // memoryBarrierCount
            nullptr, // pMemoryBarriers
            0, // bufferMemoryBarrierCount
            nullptr, // pBufferMemoryBarriers
            1, // imageMemoryBarrierCount
            &image_memory_barrier);
    }

    void CommandBuffer::next_subpass()
    {
        // Increment subpass index
        pipeline_state.set_subpass_index(pipeline_state.get_subpass_index() + 1);

        // Update blend state attachments
        auto blend_state = pipeline_state.get_color_blend_state();
        blend_state.attachments.resize(current_render_pass->get_color_output_count(pipeline_state.get_subpass_index()));
        pipeline_state.set_color_blend_state(blend_state);

        // Reset descriptor sets
        resource_binding_state.reset();
        descriptor_set_layout_binding_state.clear();

        // Clear stored push constants
        stored_push_constants.clear();

        vkCmdNextSubpass(
            this->GetHandle(), // VkCommandBuffer
            VK_SUBPASS_CONTENTS_INLINE);
    }

    void CommandBuffer::push_constants(const std::vector<uint8_t>& values)
    {
        uint32_t push_constant_size = to_u32(stored_push_constants.size() + values.size());

        if (push_constant_size > max_push_constants_size)
        {
            LOGE("Push constant limit of {} exceeded (pushing {} bytes for a total of {} bytes)",
                 max_push_constants_size, values.size(), push_constant_size);
            // TODO throw std::runtime_error("Push constant limit exceeded.");
        }
        else
        {
            stored_push_constants.insert(stored_push_constants.end(), values.begin(), values.end());
        }
    }

    VkResult CommandBuffer::reset(vkb::CommandBufferResetMode reset_mode)
    {
        return reset_impl(reset_mode);
    }

    VkResult CommandBuffer::reset_impl(vkb::CommandBufferResetMode reset_mode)
    {
        assert(
            reset_mode == command_pool.get_reset_mode() &&
            "Command buffer reset mode must match the one used by the pool to allocate it");
        if (reset_mode == vkb::CommandBufferResetMode::ResetIndividually)
        {
            vkResetCommandBuffer(this->GetHandle(), VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
        }

        return VK_SUCCESS;
    }

    void CommandBuffer::reset_query_pool(QueryPoolType const& query_pool, uint32_t first_query, uint32_t query_count)
    {
        vkCmdResetQueryPool(
            this->GetHandle(), // VkCommandBuffer commandBuffer
            query_pool.get_handle(), // VkQueryPool queryPool
            first_query, // uint32_t firstQuery
            query_count // uint32_t queryCount
        );
    }

    void CommandBuffer::resolve_image(vkb::Image const& src_img, vkb::Image const& dst_img,
                                      std::vector<ImageResolveType> const& regions)
    {
        vkCmdResolveImage(
            this->GetHandle(), // VkCommandBuffer commandBuffer
            src_img.GetHandle(), // VkImage srcImage
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, // VkImageLayout srcImageLayout
            dst_img.GetHandle(), // VkImage dstImage
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // VkImageLayout dstImageLayout
            static_cast<uint32_t>(regions.size()), // uint32_t regionCount
            regions.data() // const VkImageResolve* pRegions
        );
    }

    void CommandBuffer::set_blend_constants(std::array<float, 4> const& blend_constants)
    {
        vkCmdSetBlendConstants(
            this->GetHandle(), // VkCommandBuffer commandBuffer
            blend_constants.data() // const float blendConstants[4]
        );
    }

    void CommandBuffer::set_color_blend_state(ColorBlendStateType const& state_info)
    {
        pipeline_state.set_color_blend_state(state_info);
    }

    void CommandBuffer::set_depth_bias(float depth_bias_constant_factor, float depth_bias_clamp,
                                       float depth_bias_slope_factor)
    {
        vkCmdSetDepthBias(
            this->GetHandle(), // VkCommandBuffer commandBuffer
            depth_bias_constant_factor, // float depthBiasConstantFactor
            depth_bias_clamp, // float depthBiasClamp
            depth_bias_slope_factor // float depthBiasSlopeFactor
        );
    }

    void CommandBuffer::set_depth_bounds(float min_depth_bounds, float max_depth_bounds)
    {
        vkCmdSetDepthBounds(
            this->GetHandle(), // VkCommandBuffer commandBuffer
            min_depth_bounds, // float minDepthBounds
            max_depth_bounds // float maxDepthBounds
        );
    }

    void CommandBuffer::set_depth_stencil_state(DepthStencilStateType const& state_info)
    {
        pipeline_state.set_depth_stencil_state(state_info);
    }

    void CommandBuffer::set_input_assembly_state(InputAssemblyStateType const& state_info)
    {
        pipeline_state.set_input_assembly_state(state_info);
    }

    void CommandBuffer::set_line_width(float line_width)
    {
        vkCmdSetLineWidth(
            this->GetHandle(), // VkCommandBuffer commandBuffer
            line_width // float lineWidth
        );
    }

    void CommandBuffer::set_multisample_state(MultisampleStateType const& state_info)
    {
        pipeline_state.set_multisample_state(state_info);
    }

    void CommandBuffer::set_rasterization_state(RasterizationStateType const& state_info)
    {
        pipeline_state.set_rasterization_state(state_info);
    }

    void CommandBuffer::set_scissor(uint32_t first_scissor, std::vector<Rect2DType> const& scissors)
    {
        vkCmdSetScissor(
            this->GetHandle(), // VkCommandBuffer commandBuffer
            first_scissor, // uint32_t firstScissor
            static_cast<uint32_t>(scissors.size()), // uint32_t scissorCount
            scissors.data() // const VkRect2D* pScissors
        );
    }

    void CommandBuffer::set_specialization_constant(uint32_t constant_id, std::vector<uint8_t> const& data)
    {
        pipeline_state.set_specialization_constant(constant_id, data);
    }

    void CommandBuffer::set_update_after_bind(bool update_after_bind_)
    {
        update_after_bind = update_after_bind_;
    }

    void CommandBuffer::set_vertex_input_state(VertexInputStateType const& state_info)
    {
        pipeline_state.set_vertex_input_state(state_info);
    }

    void CommandBuffer::set_viewport(uint32_t first_viewport, std::vector<ViewportType> const& viewports)
    {
        vkCmdSetViewport(
            this->GetHandle(), // VkCommandBuffer commandBuffer
            first_viewport, // uint32_t firstViewport
            static_cast<uint32_t>(viewports.size()), // uint32_t viewportCount
            viewports.data() // const VkViewport* pViewports
        );
    }

    void CommandBuffer::set_viewport_state(ViewportStateType const& state_info)
    {
        pipeline_state.set_viewport_state(state_info);
    }

    void CommandBuffer::update_buffer(vkb::Buffer const& buffer, DeviceSizeType offset,
                                      std::vector<uint8_t> const& data)
    {
        vkCmdUpdateBuffer(
            this->GetHandle(), // VkCommandBuffer commandBuffer
            buffer.GetHandle(), // VkBuffer dstBuffer
            offset, // VkDeviceSize dstOffset
            data.size(), // VkDeviceSize dataSize
            data.data() // const void* pData
        );
    }

    void CommandBuffer::write_timestamp(VkPipelineStageFlagBits pipeline_stage, QueryPoolType const& query_pool,
                                        uint32_t query)
    {
        vkCmdWriteTimestamp(
            this->GetHandle(), // VkCommandBuffer commandBuffer
            pipeline_stage, // VkPipelineStageFlagBits pipelineStage
            query_pool.get_handle(), // VkQueryPool queryPool
            query // uint32_t query
        );
    }

    void CommandBuffer::flush(VkPipelineBindPoint pipeline_bind_point)
    {
        flush_impl(this->GetDevice(), pipeline_bind_point);
    }

    void CommandBuffer::flush_impl(vkb::VulkanDevice& device, VkPipelineBindPoint pipeline_bind_point)
    {
        flush_pipeline_state_impl(device, pipeline_bind_point);
        flush_push_constants();
        flush_descriptor_state_impl(pipeline_bind_point);
    }

    void CommandBuffer::flush_descriptor_state_impl(VkPipelineBindPoint pipeline_bind_point)
    {
        assert(command_pool.get_render_frame() && "The command pool must be associated to a render frame");

        const auto& pipeline_layout = pipeline_state.get_pipeline_layout();

        std::unordered_set<uint32_t> update_descriptor_sets;

        // Iterate over the shader sets to check if they have already been bound
        // If they have, add the set so that the command buffer later updates it
        for (auto& set_it : pipeline_layout.get_shader_sets())
        {
            uint32_t descriptor_set_id = set_it.first;

            auto descriptor_set_layout_it = descriptor_set_layout_binding_state.find(descriptor_set_id);

            if (descriptor_set_layout_it != descriptor_set_layout_binding_state.end())
            {
                if (descriptor_set_layout_it->second->get_handle() != pipeline_layout.get_descriptor_set_layout(
                        descriptor_set_id)
                    .get_handle())
                {
                    update_descriptor_sets.emplace(descriptor_set_id);
                }
            }
        }

        // Validate that the bound descriptor set layouts exist in the pipeline layout
        for (auto set_it = descriptor_set_layout_binding_state.begin(); set_it != descriptor_set_layout_binding_state.
             end();)
        {
            if (!pipeline_layout.has_descriptor_set_layout(set_it->first))
            {
                set_it = descriptor_set_layout_binding_state.erase(set_it);
            }
            else
            {
                ++set_it;
            }
        }

        // Check if a descriptor set needs to be created
        if (resource_binding_state.is_dirty() || !update_descriptor_sets.empty())
        {
            resource_binding_state.clear_dirty();

            // Iterate over all of the resource sets bound by the command buffer
            for (auto& resource_set_it : resource_binding_state.get_resource_sets())
            {
                uint32_t descriptor_set_id = resource_set_it.first;
                auto& resource_set = resource_set_it.second;

                // Don't update resource set if it's not in the update list OR its state hasn't changed
                if (!resource_set.is_dirty() && (update_descriptor_sets.find(descriptor_set_id) ==
                    update_descriptor_sets.end()))
                {
                    continue;
                }

                // Clear dirty flag for resource set
                resource_binding_state.clear_dirty(descriptor_set_id);

                // Skip resource set if a descriptor set layout doesn't exist for it
                if (!pipeline_layout.has_descriptor_set_layout(descriptor_set_id))
                {
                    continue;
                }

                auto& descriptor_set_layout = pipeline_layout.get_descriptor_set_layout(descriptor_set_id);

                // Make descriptor set layout bound for current set
                descriptor_set_layout_binding_state[descriptor_set_id] = &descriptor_set_layout;

                BindingMap<VkDescriptorBufferInfo> buffer_infos;
                BindingMap<VkDescriptorImageInfo> image_infos;

                std::vector<uint32_t> dynamic_offsets;

                // Iterate over all resource bindings
                for (auto& binding_it : resource_set.get_resource_bindings())
                {
                    auto binding_index = binding_it.first;
                    auto& binding_resources = binding_it.second;

                    // Check if binding exists in the pipeline layout
                    if (auto binding_info = descriptor_set_layout.get_layout_binding(binding_index))
                    {
                        // Iterate over all binding resources
                        for (auto& element_it : binding_resources)
                        {
                            auto array_element = element_it.first;
                            auto& resource_info = element_it.second;

                            // Pointer references
                            auto& buffer = resource_info.buffer;
                            auto& sampler = resource_info.sampler;
                            auto& image_view = resource_info.image_view;

                            // Get buffer info
                            if (buffer != nullptr && vkb::is_buffer_descriptor_type(binding_info->descriptorType))
                            {
                                VkDescriptorBufferInfo buffer_info{
                                    resource_info.buffer->GetHandle(), resource_info.offset, resource_info.range
                                };

                                if (vkb::is_dynamic_buffer_descriptor_type(binding_info->descriptorType))
                                {
                                    dynamic_offsets.push_back(to_u32(buffer_info.offset));
                                    buffer_info.offset = 0;
                                }

                                buffer_infos[binding_index][array_element] = buffer_info;
                            }

                            // Get image info
                            else if (image_view != nullptr || sampler != nullptr)
                            {
                                // Can be null for input attachments
                                VkDescriptorImageInfo image_info{
                                    sampler ? sampler->GetHandle() : nullptr, image_view->GetHandle()
                                };

                                if (image_view != nullptr)
                                {
                                    // Add image layout info based on descriptor type
                                    switch (binding_info->descriptorType)
                                    {
                                    case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                                        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                                        break;
                                    case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
                                        image_info.imageLayout = vkb::is_depth_format(image_view->get_format())
                                                                     ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
                                                                     : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                                        break;
                                    case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                                        image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                                        break;
                                    default:
                                        continue;
                                    }
                                }

                                image_infos[binding_index][array_element] = image_info;
                            }
                        }

                        assert(
                            (!update_after_bind || (buffer_infos.count(binding_index) > 0 || (image_infos.count(
                                binding_index) > 0))) &&
                            "binding index with no buffer or image infos can't be checked for adding to bindings_to_update");
                    }
                }

                VkDescriptorSet descriptor_set_handle = command_pool.get_render_frame()->request_descriptor_set(
                    descriptor_set_layout, buffer_infos, image_infos, update_after_bind,
                    command_pool.get_thread_index());

                // Bind descriptor set
                vkCmdBindDescriptorSets(
                    this->GetHandle(), // commandBuffer (VkCommandBuffer)
                    pipeline_bind_point, // pipelineBindPoint (VkPipelineBindPoint)
                    pipeline_layout.get_handle(), // layout (VkPipelineLayout)
                    descriptor_set_id, // firstSet (uint32_t)
                    1, // descriptorSetCount (uint32_t)
                    &descriptor_set_handle, // pDescriptorSets (const VkDescriptorSet*)
                    static_cast<uint32_t>(dynamic_offsets.size()), // dynamicOffsetCount (uint32_t)
                    dynamic_offsets.data() // pDynamicOffsets (const uint32_t*)
                );
            }
        }
    }

    void CommandBuffer::flush_pipeline_state_impl(vkb::VulkanDevice& device, VkPipelineBindPoint pipeline_bind_point)
    {
        // Create a new pipeline only if the graphics state changed
        if (!pipeline_state.is_dirty())
        {
            return;
        }

        pipeline_state.clear_dirty();

        // Create and bind pipeline
        if (pipeline_bind_point == VK_PIPELINE_BIND_POINT_GRAPHICS)
        {
            pipeline_state.set_render_pass(*current_render_pass);
            auto& pipeline = device.get_resource_cache().request_graphics_pipeline(pipeline_state);

            vkCmdBindPipeline(this->GetHandle(), pipeline_bind_point, pipeline.get_handle());
        }
        else if (pipeline_bind_point == VK_PIPELINE_BIND_POINT_COMPUTE)
        {
            auto& pipeline = device.get_resource_cache().request_compute_pipeline(pipeline_state);

            vkCmdBindPipeline(this->GetHandle(), pipeline_bind_point, pipeline.get_handle());
        }
        else
        {
            LOG_WARN("Only graphics and compute pipeline bind points are supported now");
        }
    }

    void CommandBuffer::flush_push_constants()
    {
        if (stored_push_constants.empty())
        {
            return;
        }

        auto const& pipeline_layout = pipeline_state.get_pipeline_layout();

        VkShaderStageFlags shader_stage = pipeline_layout.get_push_constant_range_stage(
            to_u32(stored_push_constants.size()));

        if (shader_stage)
        {
            vkCmdPushConstants(
                this->GetHandle(), // VkCommandBuffer commandBuffer
                pipeline_layout.get_handle(), // VkPipelineLayout layout
                shader_stage, // VkShaderStageFlags stageFlags
                0, // uint32_t offset
                static_cast<uint32_t>(stored_push_constants.size()), // uint32_t size
                stored_push_constants.data() // const void* pValues
            );
        }
        else
        {
            LOGW("Push constant range [{}, {}] not found", 0, stored_push_constants.size());
        }

        stored_push_constants.clear();
    }

    bool CommandBuffer::is_render_size_optimal(const VkExtent2D& framebuffer_extent, const VkRect2D& render_area)
    {
        auto render_area_granularity = current_render_pass->get_render_area_granularity();

        return ((render_area.offset.x % render_area_granularity.width == 0) && (render_area.offset.y %
                render_area_granularity.height == 0) &&
            ((render_area.extent.width % render_area_granularity.width == 0) ||
                (render_area.offset.x + render_area.extent.width == framebuffer_extent.width)) &&
            ((render_area.extent.height % render_area_granularity.height == 0) ||
                (render_area.offset.y + render_area.extent.height == framebuffer_extent.height)));
    }
}
