#pragma once

#include <volk.h>
#include <vk_mem_alloc.h>
#include "Framework/Core/Buffer.hpp"

namespace vkb
{
    class VulkanDevice;
    
    class BufferAllocation
    {
    public:
        using DeviceSizeType = VkDeviceSize;

    public:
        BufferAllocation() = default;
        BufferAllocation(const BufferAllocation &) = default;
        BufferAllocation(BufferAllocation &&) = default;
        BufferAllocation &operator=(const BufferAllocation &) = default;
        BufferAllocation &operator=(BufferAllocation &&) = default;

        BufferAllocation(Buffer &buffer, DeviceSizeType size, DeviceSizeType offset);

        bool empty() const;
        Buffer &get_buffer();
        VkDeviceSize get_offset() const;
        VkDeviceSize get_size() const;
        void update(const std::vector<uint8_t> &data, uint32_t offset = 0);
        template <typename T>
        void update(const T &value, uint32_t offset = 0);

    private:
        Buffer *buffer = nullptr;
        VkDeviceSize offset = 0;
        VkDeviceSize size = 0;
    };

    template <typename T>
    inline void BufferAllocation::update(const T &value, uint32_t offset)
    {
        update(to_bytes(value), offset);
    }

    class BufferBlock
    {
    public:
        using BufferUsageFlagsType = VkBufferUsageFlags;
        using DeviceSizeType = VkDeviceSize;

        using DeviceType = vkb::VulkanDevice;

    public:
        BufferBlock() = delete;
        BufferBlock(BufferBlock const &rhs) = delete;
        BufferBlock(BufferBlock &&rhs) = default;
        BufferBlock &operator=(BufferBlock const &rhs) = delete;
        BufferBlock &operator=(BufferBlock &&rhs) = default;

        BufferBlock(DeviceType &device, DeviceSizeType size, BufferUsageFlagsType usage, VmaMemoryUsage memory_usage);

        /**
         * @return An usable view on a portion of the underlying buffer
         */
        BufferAllocation allocate(DeviceSizeType size);

        /**
         * @brief check if this BufferBlock can allocate a given amount of memory
         * @param size the number of bytes to check
         * @return \c true if \a size bytes can be allocated from this \c BufferBlock, otherwise \c false.
         */
        bool can_allocate(DeviceSizeType size) const;

        DeviceSizeType get_size() const;
        void reset();

    private:
        /**
         * @ brief Determine the current aligned offset.
         * @return The current aligned offset.
         */
        VkDeviceSize aligned_offset() const;
        VkDeviceSize determine_alignment(VkBufferUsageFlags usage, VkPhysicalDeviceLimits const &limits) const;

    private:
        vkb::Buffer buffer;
        VkDeviceSize alignment = 0; // Memory alignment, it may change according to the usage
        VkDeviceSize offset = 0;    // Current offset, it increases on every allocation
    };

    class BufferPool
    {
    public:
        using BufferUsageFlagsType = VkBufferUsageFlags;
        using DeviceSizeType = VkDeviceSize;

        using DeviceType = vkb::VulkanDevice;

    public:
        BufferPool(DeviceType &device, DeviceSizeType block_size, BufferUsageFlagsType usage, VmaMemoryUsage memory_usage = VMA_MEMORY_USAGE_CPU_TO_GPU);

        BufferBlock &request_buffer_block(DeviceSizeType minimum_size, bool minimal = false);

        void reset();

    private:
        VulkanDevice &device;
        std::vector<std::unique_ptr<BufferBlock>> buffer_blocks; /// List of blocks requested (need to be pointers in order to keep their address constant on vector resizing)
        VkDeviceSize block_size = 0;                           /// Minimum size of the blocks
        VkBufferUsageFlags usage;
        VmaMemoryUsage memory_usage{};
    };

}
