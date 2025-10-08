#include "Framework/Misc/BufferPool.hpp"
#include "Framework/Core/VulkanDevice.hpp"

namespace vkb
{
    BufferAllocation::BufferAllocation(Buffer& buffer, DeviceSizeType size, DeviceSizeType offset)
        : buffer(&buffer), offset(offset), size(size)
    {
    }

    bool BufferAllocation::empty() const
    {
        return size == 0 || buffer == nullptr;
    }

    Buffer& BufferAllocation::get_buffer()
    {
        return *buffer;
    }

    VkDeviceSize BufferAllocation::get_offset() const
    {
        return offset;
    }

    VkDeviceSize BufferAllocation::get_size() const
    {
        return size;
    }

    void BufferAllocation::update(const std::vector<uint8_t>& data, uint32_t offset)
    {
        assert(buffer && "Invalid buffer pointer");

        if (offset + data.size() <= size)
        {
            buffer->update(data, to_u32(this->offset) + offset);
        }
        else
        {
            // TODO LOGE("Ignore buffer allocation update");
        }
    }

    BufferBlock::BufferBlock(DeviceType& device, DeviceSizeType size, BufferUsageFlagsType usage,
                             VmaMemoryUsage memory_usage)
        : buffer{device, size, usage, memory_usage}
    {
        alignment = determine_alignment(usage, device.get_gpu().get_properties().limits);
    }

    BufferAllocation BufferBlock::allocate(DeviceSizeType size)
    {
        if (can_allocate(size))
        {
            // Move the current offset and return an allocation
            auto aligned = aligned_offset();
            offset = aligned + size;
            return BufferAllocation{
                reinterpret_cast<vkb::Buffer&>(buffer), static_cast<VkDeviceSize>(size),
                static_cast<VkDeviceSize>(aligned)
            };
        }
        // No more space available from the underlying buffer, return empty allocation
        return BufferAllocation{};
    }

    bool BufferBlock::can_allocate(DeviceSizeType size) const
    {
        assert(size > 0 && "Allocation size must be greater than zero");
        return (aligned_offset() + size <= buffer.get_size());
    }

    BufferBlock::DeviceSizeType BufferBlock::get_size() const
    {
        return buffer.get_size();
    }

    void BufferBlock::reset()
    {
        offset = 0;
    }

    VkDeviceSize BufferBlock::aligned_offset() const
    {
        return (offset + alignment - 1) & ~(alignment - 1);
    }

    VkDeviceSize BufferBlock::determine_alignment(VkBufferUsageFlags usage, VkPhysicalDeviceLimits const& limits) const
    {
        if (usage == VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
        {
            return limits.minUniformBufferOffsetAlignment;
        }
        else if (usage == VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
        {
            return limits.minStorageBufferOffsetAlignment;
        }
        else if (usage == VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT)
        {
            return limits.minTexelBufferOffsetAlignment;
        }
        else if (usage == VK_BUFFER_USAGE_INDEX_BUFFER_BIT ||
            usage == VK_BUFFER_USAGE_VERTEX_BUFFER_BIT ||
            usage == VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT)
        {
            // Used to calculate the offset, required when allocating memory (its value should be power of 2)
            return 16;
        }
        else
        {
            throw std::runtime_error("Usage not recognised");
        }
    }

    BufferPool::BufferPool(DeviceType& device, DeviceSizeType block_size, BufferUsageFlagsType usage,
                           VmaMemoryUsage memory_usage)
        : device{device}, block_size(block_size), usage(usage), memory_usage(memory_usage)
    {
    }

    BufferBlock& BufferPool::request_buffer_block(DeviceSizeType minimum_size, bool minimal)
    {
        // Find a block in the range of the blocks which can fit the minimum size
        auto it = minimal
                      ? std::find_if(buffer_blocks.begin(), buffer_blocks.end(),
                                     [&minimum_size](auto const& buffer_block)
                                     {
                                         return (buffer_block->get_size() == minimum_size) && buffer_block->
                                             can_allocate(minimum_size);
                                     })
                      : std::find_if(buffer_blocks.begin(), buffer_blocks.end(),
                                     [&minimum_size](auto const& buffer_block)
                                     {
                                         return buffer_block->can_allocate(minimum_size);
                                     });
        if (it == buffer_blocks.end())
        {
            // TODO LOGD("Building #{} buffer block ({})", buffer_blocks.size(), vk::to_string(usage));

            VkDeviceSize new_block_size = minimal ? minimum_size : std::max(block_size, minimum_size);

            // Create a new block and get the iterator on it
            it = buffer_blocks.emplace(buffer_blocks.end(),
                                       std::make_unique<BufferBlock>(device, new_block_size, usage, memory_usage));
        }
        return *it->get();
    }

    void BufferPool::reset()
    {
        for (auto& buffer_block : buffer_blocks)
        {
            buffer_block->reset();
        }
    }
}
