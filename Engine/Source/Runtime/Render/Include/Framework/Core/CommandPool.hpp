#pragma once

#include <cstdint>

#include "Framework/Common/VkCommon.hpp"

namespace vkb
{
    class VulkanDevice;
    class RenderFrame;
    class CommandBuffer;
    class CommandPool;

    class CommandPool
    {
    public:
        CommandPool(vkb::VulkanDevice &device,
                    uint32_t queue_family_index,
                    vkb::RenderFrame *render_frame = nullptr,
                    size_t thread_index = 0,
                    vkb::CommandBufferResetMode reset_mode = vkb::CommandBufferResetMode::ResetPool);
        CommandPool(CommandPool const &) = delete;
        CommandPool(CommandPool &&other) noexcept;
        CommandPool &operator=(CommandPool const &) = delete;
        CommandPool &operator=(CommandPool &&other) = delete;
        ~CommandPool();

        vkb::VulkanDevice &get_device();
        VkCommandPool get_handle() const;
        uint32_t get_queue_family_index() const;
        vkb::RenderFrame *get_render_frame();
        vkb::CommandBufferResetMode get_reset_mode() const;
        size_t get_thread_index() const;

        std::shared_ptr<vkb::CommandBuffer> request_command_buffer(
            VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        std::shared_ptr<vkb::CommandBuffer> request_command_buffer(vkb::CommandPool &commandPool,
                                                                   VkCommandBufferLevel level);
        void reset_pool();

    private:
        vkb::VulkanDevice &device;
        VkCommandPool handle = nullptr;
        vkb::RenderFrame *render_frame = nullptr;
        size_t thread_index = 0;
        uint32_t queue_family_index = 0;
        std::vector<std::shared_ptr<vkb::CommandBuffer>> primary_command_buffers;
        uint32_t active_primary_command_buffer_count = 0;
        std::vector<std::shared_ptr<vkb::CommandBuffer>> secondary_command_buffers;
        uint32_t active_secondary_command_buffer_count = 0;
        vkb::CommandBufferResetMode reset_mode = vkb::CommandBufferResetMode::ResetPool;
    };
} // namespace vkb

namespace vks
{
    class CommandPool
    {
    public:
        CommandPool(VkDevice device, VkCommandPool commandPool);
        ~CommandPool();

        CommandPool(const CommandPool &) = delete;
        CommandPool &operator=(const CommandPool &) = delete;

        CommandPool(CommandPool &&other) noexcept;
        CommandPool &operator=(CommandPool &&other) noexcept;

        /**
         * @brief Allocates one or more command buffers from the pool.
         * @param count The number of command buffers to allocate.
         * @param level The level of the command buffers (PRIMARY or SECONDARY).
         * @return A vector containing the allocated VkCommandBuffer handles. Throws an exception if allocation fails.
         * */
        [[nodiscard]] std::vector<VkCommandBuffer> allocateCommandBuffers(
            uint32_t count, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

        void freeCommandBuffers(const std::vector<VkCommandBuffer> &commandBuffers);

        void reset(VkCommandPoolResetFlags flags = 0);

        VkCommandPool get() const { return m_commandPool; }
        operator VkCommandPool() const { return m_commandPool; }

    private:
        void destroy();

        VkDevice m_device = VK_NULL_HANDLE;
        VkCommandPool m_commandPool = VK_NULL_HANDLE;
    };

    class CommandPoolBuilder
    {
    public:
        explicit CommandPoolBuilder(VkDevice device);

        CommandPoolBuilder &setQueueFamilyIndex(uint32_t queueFamilyIndex);

        CommandPoolBuilder &setFlags(VkCommandPoolCreateFlags flags);

        CommandPoolBuilder &setTransient();

        CommandPoolBuilder &setResetCommandBuffer();

        [[nodiscard]] CommandPool build() const;

        [[nodiscard]] VkCommandPoolCreateInfo buildCreateInfo() const;

    private:
        VkDevice m_device;
        VkCommandPoolCreateInfo m_createInfo{};
    };
}
