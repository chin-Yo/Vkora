#include "Rendering/ComputePipeline/ComputePass.hpp"

#include <iostream>

#include "Framework/Core/Buffer.hpp"
#include "Framework/Core/VulkanDevice.hpp"
#include "Framework/Core/CommandBuffer.hpp"
#include "Framework/Core/CommandPool.hpp"
#include "Framework/Core/ImageView.hpp"
#include "Framework/Core/Queue.hpp"
#include "Framework/Core/Sampler.hpp"

ComputePassBase::ComputePassBase(vkb::VulkanDevice& device, vkb::ShaderSource&& compute_shader)
    : device(device), computeShader(std::move(compute_shader))
{
}

ComputePassBase::~ComputePassBase()
{
}

void ComputePassBase::Dispatch(uint32_t x, uint32_t y, uint32_t z,
                               RecordingFunction recordFn,
                               TaskCallback callback)
{
    ComputeTaskContext task;
    task.onComplete = callback;
    task.fence = device.request_fence();

    auto& resource_cache = device.get_resource_cache();
    auto& cs = resource_cache.request_shader_module(VK_SHADER_STAGE_COMPUTE_BIT, computeShader);

    std::vector<vkb::ShaderModule*> shader_modules{&cs};
    auto& pipeline_layout = resource_cache.request_pipeline_layout(shader_modules);
    vkb::PipelineState ps;
    ps.set_pipeline_layout(pipeline_layout);
    auto& cp = resource_cache.request_compute_pipeline(ps);

    task.cmdBuffer = device.create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

    vkb::ResourceBindingState RBS;
    // 让用户绑定特定的 DescriptorSet 或 PushConstants
    if (recordFn)
    {
        recordFn(RBS);
    }
    vkCmdBindPipeline(task.cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, cp.get_handle());

    // Iterate over all of the resource sets bound by the command buffer
    for (auto& resource_set_it : RBS.get_resource_sets())
    {
        uint32_t descriptor_set_id = resource_set_it.first;
        auto& resource_set = resource_set_it.second;

        // Clear dirty flag for resource set
        RBS.clear_dirty(descriptor_set_id);

        // Skip resource set if a descriptor set layout doesn't exist for it
        if (!pipeline_layout.has_descriptor_set_layout(descriptor_set_id))
        {
            continue;
        }

        auto& descriptor_set_layout = pipeline_layout.get_descriptor_set_layout(descriptor_set_id);

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
                            dynamic_offsets.push_back(vkb::to_u32(buffer_info.offset));
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
            }
        }
        auto& ds = resource_cache.request_descriptor_set(pipeline_layout.get_descriptor_set_layout(0), buffer_infos,
                                                         image_infos);
        ds.apply_writes();
        VkDescriptorSet handle = ds.get_handle();
        vkCmdBindDescriptorSets(
            task.cmdBuffer, // commandBuffer (VkCommandBuffer)
            VK_PIPELINE_BIND_POINT_COMPUTE, // pipelineBindPoint (VkPipelineBindPoint)
            pipeline_layout.get_handle(), // layout (VkPipelineLayout)
            descriptor_set_id, // firstSet (uint32_t)
            1, // descriptorSetCount (uint32_t)
            &handle, // pDescriptorSets (const VkDescriptorSet*)
            0, nullptr
        );
    }

    auto index = device.get_queue_family_index(VK_QUEUE_COMPUTE_BIT);
    auto& queue = device.get_queue_by_flags(VK_QUEUE_COMPUTE_BIT, index);
    // 4. 提交
    VkSubmitInfo submitInfo = vks::initializers::submitInfo();
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &task.cmdBuffer;
    vkEndCommandBuffer(task.cmdBuffer);
    VK_CHECK_RESULT(vkQueueSubmit(queue.get_handle(), 1, &submitInfo, task.fence));
    // 5. 加入队列
    pendingTasks.push_back(task);
}

void ComputePassBase::Poll()
{
    if (pendingTasks.empty()) return;

    // 检查队列头部的任务是否完成
    // 注意：如果前面的任务没完成，后面的即使完成了也不会被处理（保持顺序），也可以改为遍历处理。
    auto& task = pendingTasks.front();

    VkResult result = vkGetFenceStatus(device.GetHandle(), task.fence);
    if (result == VK_SUCCESS)
    {
        // 1. 执行回调 (例如保存图片)
        if (task.onComplete)
        {
            task.onComplete();
        }
        // 2. 清理临时资源
        vkDestroyFence(device.GetHandle(), task.fence, nullptr);
        // 3. 移除任务
        pendingTasks.pop_front();
    }
}
