#pragma once

#include <volk.h>
#include <map>
#include <vector>
#include <unordered_map>
#include <memory>

#include "Framework/Misc/BufferPool.hpp"
#include "Framework/Misc/FencePool.hpp"
#include "Framework/Misc/SemaphorePool.hpp"


namespace vkb
{
    class DescriptorSetLayout;
    class BufferBlock;
    class BufferPool;
    class DescriptorPool;
    class DescriptorSet;
    class RenderTarget;
    class VulkanDevice;
    class Queue;
    class CommandPool;
    
    enum BufferAllocationStrategy
    {
        OneAllocationPerBuffer,
        MultipleAllocationsPerBuffer
    };

    enum DescriptorManagementStrategy
    {
        StoreInCache,
        CreateDirectly
    };

    class RenderFrame
    {
    public:
        using BufferUsageFlagsType = VkBufferUsageFlags;
        using CommandBufferLevelType = VkCommandBufferLevel;
        using DescriptorBufferInfoType = VkDescriptorBufferInfo;
        using DescriptorImageInfoType = VkDescriptorImageInfo;
        using DescriptorSetType = VkDescriptorSet;
        using DeviceSizeType = VkDeviceSize;
        using FenceType = VkFence;
        using SemaphoreType = VkSemaphore;

        using DeviceType = vkb::VulkanDevice;
        using DescriptorSetLayoutType = vkb::DescriptorSetLayout;
        using FencePoolType = vkb::FencePool;
        using QueueType = vkb::Queue;
        using RenderTargetType = vkb::RenderTarget;
        using SemaphorePoolType = vkb::SemaphorePool;

    public:
        RenderFrame(DeviceType &device, std::unique_ptr<RenderTargetType> &&render_target, size_t thread_count = 1);
        RenderFrame(RenderFrame const &) = delete;
        RenderFrame(RenderFrame &&) = default;
        RenderFrame &operator=(RenderFrame const &) = delete;
        RenderFrame &operator=(RenderFrame &&) = default;

        /**
         * @param usage Usage of the buffer
         * @param size Amount of memory required
         * @param thread_index Index of the buffer pool to be used by the current thread
         * @return The requested allocation, it may be empty
         */
        vkb::BufferAllocation allocate_buffer(BufferUsageFlagsType usage, DeviceSizeType size, size_t thread_index = 0);

        void clear_descriptors();

        /**
         * @brief Get the command pool of the active frame
         *        A frame should be active at the moment of requesting it
         * @param queue The queue command buffers will be submitted on
         * @param reset_mode Indicate how the command buffer will be used, may trigger a pool re-creation to set necessary flags
         * @param thread_index Selects the thread's command pool used to manage the buffer
         * @return The command pool related to the current active frame
         */
        vkb::CommandPool &get_command_pool(
            QueueType const &queue, vkb::CommandBufferResetMode reset_mode = vkb::CommandBufferResetMode::ResetPool, size_t thread_index = 0);

        DeviceType &get_device();
        FencePoolType &get_fence_pool();
        FencePoolType const &get_fence_pool() const;
        RenderTargetType &get_render_target();
        RenderTargetType const &get_render_target() const;
        SemaphorePoolType &get_semaphore_pool();
        SemaphorePoolType const &get_semaphore_pool() const;
        DescriptorSetType request_descriptor_set(DescriptorSetLayoutType const &descriptor_set_layout,
                                                 BindingMap<DescriptorBufferInfoType> const &buffer_infos,
                                                 BindingMap<DescriptorImageInfoType> const &image_infos,
                                                 bool update_after_bind,
                                                 size_t thread_index = 0);
        void reset();

        /**
         * @brief Sets a new buffer allocation strategy
         * @param new_strategy The new buffer allocation strategy
         */
        void set_buffer_allocation_strategy(BufferAllocationStrategy new_strategy);

        /**
         * @brief Sets a new descriptor set management strategy
         * @param new_strategy The new descriptor set management strategy
         */
        void set_descriptor_management_strategy(DescriptorManagementStrategy new_strategy);

        /**
         * @brief Updates all the descriptor sets in the current frame at a specific thread index
         */
        void update_descriptor_sets(size_t thread_index = 0);

        /**
         * @brief Called when the swapchain changes
         * @param render_target A new render target with updated images
         */
        void update_render_target(std::unique_ptr<RenderTargetType> &&render_target);

    private:
        vkb::BufferAllocation allocate_buffer_impl(VkBufferUsageFlags usage, VkDeviceSize size, size_t thread_index);
        vkb::CommandPool &get_command_pool_impl(vkb::Queue const &queue, vkb::CommandBufferResetMode reset_mode, size_t thread_index);

        /**
         * @brief Retrieve the frame's command pool(s)
         * @param queue The queue command buffers will be submitted on
         * @param reset_mode Indicate how the command buffers will be reset after execution,
         *        may trigger a pool re-creation to set necessary flags
         * @return The frame's command pool(s)
         */
        std::vector<vkb::CommandPool> &get_command_pools(const vkb::Queue &queue, vkb::CommandBufferResetMode reset_mode);

        VkDescriptorSet request_descriptor_set_impl(vkb::DescriptorSetLayout const &descriptor_set_layout,
                                                      BindingMap<VkDescriptorBufferInfo> const &buffer_infos,
                                                      BindingMap<VkDescriptorImageInfo> const &image_infos,
                                                      bool update_after_bind,
                                                      size_t thread_index = 0);

    private:
        VulkanDevice &device;
        std::map<VkBufferUsageFlags, std::vector<std::pair<vkb::BufferPool, vkb::BufferBlock *>>> buffer_pools;
        std::map<uint32_t, std::vector<vkb::CommandPool>> command_pools;                    // Commands pools per queue family index
        std::vector<std::unordered_map<std::size_t, vkb::DescriptorPool>> descriptor_pools; // Descriptor pools per thread
        std::vector<std::unordered_map<std::size_t, vkb::DescriptorSet>> descriptor_sets;   // Descriptor sets per thread
        vkb::FencePool fence_pool;
        vkb::SemaphorePool semaphore_pool;
        std::unique_ptr<vkb::RenderTarget> swapchain_render_target;
        size_t thread_count;
        BufferAllocationStrategy buffer_allocation_strategy = BufferAllocationStrategy::MultipleAllocationsPerBuffer;
        DescriptorManagementStrategy descriptor_management_strategy = DescriptorManagementStrategy::StoreInCache;
    };
}
