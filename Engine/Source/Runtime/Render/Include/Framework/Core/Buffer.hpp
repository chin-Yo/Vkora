#pragma once

#include "Framework/Core/Allocated.hpp"
#include "Framework/Misc/BuilderBase.hpp"
#include <memory>
#include <vector>

class vkb::VulkanDevice;

namespace vkb
{
    class Buffer;

    class BufferBuilder
        : public BuilderBase<BufferBuilder, VkBufferCreateInfo>
    {
    private:
        using ParentType = BuilderBase<BufferBuilder, VkBufferCreateInfo>;

    public:
        BufferBuilder(VkDeviceSize size)
            : ParentType(VkBufferCreateInfo{
                  VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, // sType
                  nullptr,                              // pNext
                  0,                                    // flags
                  size,                                 // size
                  0,                                    // usage
                  VK_SHARING_MODE_EXCLUSIVE,            // sharingMode
                  0,                                    // queueFamilyIndexCount
                  nullptr                               // pQueueFamilyIndices
              })
        {
        }

        Buffer build(VulkanDevice &device) const;
        std::unique_ptr<Buffer> build_unique(VulkanDevice &device) const;

        BufferBuilder &with_flags(VkBufferCreateFlags flags)
        {
            this->get_create_info().flags = flags;
            return *this;
        }

        BufferBuilder &with_usage(VkBufferUsageFlags usage)
        {
            this->get_create_info().usage = usage;
            return *this;
        }
    };

    class Buffer : public Allocated<VkBuffer>
    {
    private:
        using ParentType = Allocated<VkBuffer>;

    public:
        static Buffer create_staging_buffer(VulkanDevice &device, VkDeviceSize size, const void *data);

        template <typename T>
        static Buffer create_staging_buffer(VulkanDevice &device, const std::vector<T> &data);

        template <typename T>
        static Buffer create_staging_buffer(VulkanDevice &device, const T &data);

        Buffer() = delete;
        Buffer(const Buffer &) = delete;
        Buffer(Buffer &&other) = default;
        Buffer &operator=(const Buffer &) = delete;
        Buffer &operator=(Buffer &&) = default;
        ~Buffer() override;

        Buffer(DeviceType &device,
               VkDeviceSize size,
               VkBufferUsageFlags buffer_usage,
               VmaMemoryUsage memory_usage,
               VmaAllocationCreateFlags flags = VMA_ALLOCATION_CREATE_MAPPED_BIT |
                                                VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT,
               const std::vector<uint32_t> &queue_family_indices = {});

        Buffer(DeviceType &device, const BufferBuilder &builder);

        uint64_t get_device_address() const;

        VkDeviceSize get_size() const { return size; }

    private:
        VkDeviceSize size = 0;
    };

    inline Buffer BufferBuilder::build(VulkanDevice &device) const
    {
        return Buffer{device, *this};
    }

    inline std::unique_ptr<Buffer> BufferBuilder::build_unique(VulkanDevice &device) const
    {
        return std::make_unique<Buffer>(device, *this);
    }

    template <typename T>
    Buffer Buffer::create_staging_buffer(DeviceType &device, const T &data)
    {
        return create_staging_buffer(device, sizeof(T), &data);
    }

    template <typename T>
    Buffer Buffer::create_staging_buffer(DeviceType &device, const std::vector<T> &data)
    {
        return create_staging_buffer(device, data.size() * sizeof(T), data.data());
    }
} // namespace vkb
