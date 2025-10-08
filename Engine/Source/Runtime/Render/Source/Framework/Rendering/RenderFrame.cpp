#include "Framework/Rendering/RenderFrame.hpp"
#include "Framework/Core/CommandPool.hpp"
#include "Framework/Core/Queue.hpp"
#include "Framework/Common/ResourceCaching.hpp"
#include "Framework/Core/VulkanDevice.hpp"

namespace vkb
{
    RenderFrame::RenderFrame(vkb::VulkanDevice &device_, std::unique_ptr<RenderTarget> &&render_target,
                             size_t thread_count)
        : device(device_),
          fence_pool{device},
          semaphore_pool{device},
          thread_count{thread_count},
          descriptor_pools(thread_count),
          descriptor_sets(thread_count)
    {
        static constexpr uint32_t BUFFER_POOL_BLOCK_SIZE = 256; // Block size of a buffer pool in kilobytes

        // A map of the supported usages to a multiplier for the BUFFER_POOL_BLOCK_SIZE
        static const std::unordered_map<VkBufferUsageFlags, uint32_t> supported_usage_map = {
            {VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 1},
            {VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 2},
            // x2 the size of BUFFER_POOL_BLOCK_SIZE since SSBOs are normally much larger than other types of buffers
            {VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 1},
            {VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 1}};

        update_render_target(std::move(render_target));
        for (auto &usage_it : supported_usage_map)
        {
            auto [buffer_pools_it, inserted] = buffer_pools.emplace(usage_it.first,
                                                                    std::vector<std::pair<
                                                                        vkb::BufferPool, vkb::BufferBlock *>>{});
            if (!inserted)
            {
                // throw std::runtime_error("Failed to insert buffer pool");
                // TODO: LOG("Failed to insert buffer pool");
                return;
            }

            for (size_t i = 0; i < thread_count; ++i)
            {
                buffer_pools_it->second.push_back(
                    std::make_pair(vkb::BufferPool{
                                       device, BUFFER_POOL_BLOCK_SIZE * 1024 * usage_it.second, usage_it.first},
                                   nullptr));
            }
        }
    }

    vkb::BufferAllocation RenderFrame::allocate_buffer(VkBufferUsageFlags usage, VkDeviceSize size, size_t thread_index)
    {
        assert(thread_index < thread_count && "Thread index is out of bounds");
        return allocate_buffer_impl(usage, size, thread_index);
    }

    vkb::BufferAllocation RenderFrame::allocate_buffer_impl(VkBufferUsageFlags usage, VkDeviceSize size,
                                                            size_t thread_index)
    {
        // Find a pool for this usage
        auto buffer_pool_it = buffer_pools.find(usage);
        if (buffer_pool_it == buffer_pools.end())
        {
            LOGE("No buffer pool for buffer usage {} ", vkb::to_string(usage))
            return vkb::BufferAllocation{};
        }

        assert(thread_index < buffer_pool_it->second.size());
        auto &buffer_pool = buffer_pool_it->second[thread_index].first;
        auto &buffer_block = buffer_pool_it->second[thread_index].second;

        bool want_minimal_block = (buffer_allocation_strategy == BufferAllocationStrategy::OneAllocationPerBuffer);

        if (want_minimal_block || !buffer_block || !buffer_block->can_allocate(size))
        {
            // If we are creating a buffer for each allocation of there is no block associated with the pool or the current block is too small
            // for this allocation, request a new buffer block
            buffer_block = &buffer_pool.request_buffer_block(size, want_minimal_block);
        }

        return buffer_block->allocate(static_cast<uint32_t>(size));
    }

    void RenderFrame::clear_descriptors()
    {
        for (auto &desc_sets_per_thread : descriptor_sets)
        {
            desc_sets_per_thread.clear();
        }

        for (auto &desc_pools_per_thread : descriptor_pools)
        {
            for (auto &desc_pool : desc_pools_per_thread)
            {
                desc_pool.second.reset();
            }
        }
    }

    std::vector<vkb::CommandPool> &RenderFrame::get_command_pools(const vkb::Queue &queue,
                                                                  vkb::CommandBufferResetMode reset_mode)
    {
        auto command_pool_it = command_pools.find(queue.get_family_index());

        if (command_pool_it != command_pools.end())
        {
            assert(!command_pool_it->second.empty());
            if (command_pool_it->second[0].get_reset_mode() != reset_mode)
            {
                vkDeviceWaitIdle(device.GetHandle());

                // Delete pools
                command_pools.erase(command_pool_it);
            }
            else
            {
                return command_pool_it->second;
            }
        }

        bool inserted = false;
        std::tie(command_pool_it, inserted) = command_pools.emplace(queue.get_family_index(),
                                                                    std::vector<vkb::CommandPool>{});
        if (!inserted)
        {
            throw std::runtime_error("Failed to insert command pool");
        }

        for (size_t i = 0; i < thread_count; i++)
        {
            command_pool_it->second.emplace_back(device, queue.get_family_index(),
                                                 reinterpret_cast<vkb::RenderFrame *>(this), i, reset_mode);
        }

        return command_pool_it->second;
    }

    vkb::CommandPool &RenderFrame::get_command_pool(const vkb::Queue &queue, vkb::CommandBufferResetMode reset_mode,
                                                    size_t thread_index)
    {
        assert(thread_index < thread_count && "Thread index is out of bounds");

        auto &command_pools = get_command_pools(queue, reset_mode);

        auto command_pool_it = std::find_if(
            command_pools.begin(), command_pools.end(),
            [&thread_index](const vkb::CommandPool &cmd_pool)
            {
                return cmd_pool.get_thread_index() == thread_index;
            });

        assert(command_pool_it != command_pools.end() && "Command pool for the given thread index not found");

        return *command_pool_it;
    }

    vkb::VulkanDevice &RenderFrame::get_device()
    {
        return device;
    }

    vkb::FencePool &RenderFrame::get_fence_pool()
    {
        return fence_pool;
    }

    const vkb::FencePool &RenderFrame::get_fence_pool() const
    {
        return fence_pool;
    }

    vkb::RenderTarget &RenderFrame::get_render_target()
    {
        return *swapchain_render_target;
    }

    const vkb::RenderTarget &RenderFrame::get_render_target() const
    {
        return *swapchain_render_target;
    }

    vkb::SemaphorePool &RenderFrame::get_semaphore_pool()
    {
        return semaphore_pool;
    }

    const vkb::SemaphorePool &RenderFrame::get_semaphore_pool() const
    {
        return semaphore_pool;
    }

    VkDescriptorSet RenderFrame::request_descriptor_set(const vkb::DescriptorSetLayout &descriptor_set_layout,
                                                        const BindingMap<VkDescriptorBufferInfo> &buffer_infos,
                                                        const BindingMap<VkDescriptorImageInfo> &image_infos,
                                                        bool update_after_bind,
                                                        size_t thread_index)
    {
        assert(thread_index < thread_count && "Thread index is out of bounds");
        assert(thread_index < descriptor_pools.size());

        auto &descriptor_pool = vkb::request_resource(device, nullptr, descriptor_pools[thread_index],
                                                      descriptor_set_layout);
        if (descriptor_management_strategy == DescriptorManagementStrategy::StoreInCache)
        {
            // The bindings we want to update before binding, if empty we update all bindings
            std::set<uint32_t> bindings_to_update;
            // If update after bind is enabled, we store the binding index of each binding that need to be updated before being bound
            if (update_after_bind)
            {
                auto aggregate_binding_to_update = [&bindings_to_update, &descriptor_set_layout](const auto &infos_map)
                {
                    for (const auto &[binding_index, ignored] : infos_map)
                    {
                        if (!(descriptor_set_layout.get_layout_binding_flag(binding_index) &
                              VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT))
                        {
                            bindings_to_update.insert(binding_index);
                        }
                    }
                };
                aggregate_binding_to_update(buffer_infos);
                aggregate_binding_to_update(image_infos);
            }

            // Request a descriptor set from the render frame, and write the buffer infos and image infos of all the specified bindings
            assert(thread_index < descriptor_sets.size());
            auto &descriptor_set =
                vkb::request_resource(device, nullptr, descriptor_sets[thread_index], descriptor_set_layout,
                                      descriptor_pool, buffer_infos, image_infos);
            descriptor_set.update({bindings_to_update.begin(), bindings_to_update.end()});
            return descriptor_set.get_handle();
        }
        else
        {
            // Request a descriptor pool, allocate a descriptor set, write buffer and image data to it
            vkb::DescriptorSet descriptor_set{
                device, descriptor_set_layout, descriptor_pool, buffer_infos, image_infos};
            descriptor_set.apply_writes();
            return descriptor_set.get_handle();
        }
    }

    void RenderFrame::reset()
    {
        VK_CHECK_RESULT(fence_pool.wait());
        fence_pool.reset();

        for (auto &command_pools_per_queue : command_pools)
        {
            for (auto &command_pool : command_pools_per_queue.second)
            {
                command_pool.reset_pool();
            }
        }

        for (auto &buffer_pools_per_usage : buffer_pools)
        {
            for (auto &buffer_pool : buffer_pools_per_usage.second)
            {
                buffer_pool.first.reset();
                buffer_pool.second = nullptr;
            }
        }

        semaphore_pool.reset();

        if (descriptor_management_strategy == DescriptorManagementStrategy::CreateDirectly)
        {
            clear_descriptors();
        }
    }

    void RenderFrame::set_buffer_allocation_strategy(BufferAllocationStrategy new_strategy)
    {
        buffer_allocation_strategy = new_strategy;
    }

    void RenderFrame::set_descriptor_management_strategy(DescriptorManagementStrategy new_strategy)
    {
        descriptor_management_strategy = new_strategy;
    }

    void RenderFrame::update_descriptor_sets(size_t thread_index)
    {
        assert(thread_index < descriptor_sets.size());

        auto &thread_descriptor_sets = descriptor_sets[thread_index];
        for (auto &descriptor_set_it : thread_descriptor_sets)
        {
            descriptor_set_it.second.update();
        }
    }

    void RenderFrame::update_render_target(std::unique_ptr<RenderTarget> &&render_target)
    {
        swapchain_render_target = std::move(render_target);
    }
}
