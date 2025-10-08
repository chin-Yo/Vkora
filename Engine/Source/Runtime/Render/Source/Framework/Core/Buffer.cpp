#include "Framework/Core/Buffer.hpp"
#include "Framework/Core/VulkanDevice.hpp"

namespace vkb
{
    Buffer Buffer::create_staging_buffer(DeviceType &device, DeviceSizeType size, const void *data)
    {
        BufferBuilder builder(size);
        builder.with_vma_flags(
                   VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT)
            .with_usage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
            .with_vma_usage(VMA_MEMORY_USAGE_AUTO);

        Buffer staging_buffer(device, builder);

        if (data != nullptr)
        {
            staging_buffer.update(data, size);
        }
        return staging_buffer;
    }

    Buffer::Buffer(DeviceType &device,
                   DeviceSizeType size,
                   VkBufferUsageFlags buffer_usage,
                   VmaMemoryUsage memory_usage,
                   VmaAllocationCreateFlags flags,
                   const std::vector<uint32_t> &queue_family_indices) : Buffer(device,
                                                                               BufferBuilder(size)
                                                                                   .with_usage(buffer_usage)
                                                                                   .with_vma_usage(memory_usage)
                                                                                   .with_vma_flags(flags)
                                                                                   .with_queue_families(queue_family_indices)
                                                                                   .with_implicit_sharing_mode())
    {
    }

    Buffer::Buffer(DeviceType &device, const BufferBuilder &builder) : ParentType(builder.get_allocation_create_info(), nullptr, &device),
                                                                       size(builder.get_create_info().size)
    {
        this->SetHandle(this->create_buffer(builder.get_create_info()));

        if (!builder.get_debug_name().empty())
        {
            this->SetDebugName(builder.get_debug_name());
        }
    }

    Buffer::~Buffer()
    {
        this->destroy_buffer(this->GetHandle());
    }

#if defined(VK_VERSION_1_2) || defined(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME)
    uint64_t Buffer::get_device_address() const
    {
        assert(vkGetBufferDeviceAddress != nullptr && "vkGetBufferDeviceAddress function pointer is null!");

        VkBufferDeviceAddressInfo info{};
        info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        info.buffer = this->GetHandle();

        return vkGetBufferDeviceAddress(this->GetDevice().GetHandle(), &info);
    }
#else
    uint64_t Buffer::get_device_address() const
    {
        return 0;
    }
#endif
} // namespace vkb
