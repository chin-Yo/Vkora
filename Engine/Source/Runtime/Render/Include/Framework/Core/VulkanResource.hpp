#pragma once

#include "Framework/Common/VkCommon.hpp"

#include "Framework/Core/Debug.hpp"

namespace vkb
{
    class VulkanDevice;

    template <typename Handle>
    class VulkanResource
    {
    public:
        VulkanResource(Handle handle = nullptr, VulkanDevice* device_ = nullptr);

        VulkanResource(const VulkanResource&) = delete;
        VulkanResource& operator=(const VulkanResource&) = delete;

        VulkanResource(VulkanResource&& other);
        VulkanResource& operator=(VulkanResource&& other);
        virtual ~VulkanResource() = default;

        const std::string& GetDebugName() const;
        VulkanDevice& GetDevice();
        VulkanDevice const& GetDevice() const;
        Handle& GetHandle();
        const Handle& GetHandle() const;
        uint64_t GetHandleU64() const;
        VkObjectType GetObjectType() const;
        bool HasDevice() const;
        bool HasHandle() const;
        void SetDebugName(const std::string& name);
        void SetHandle(Handle hdl);

    private:
        std::string debug_name;
        VulkanDevice* device;
        Handle handle;
    };

    template <typename Handle>
    VulkanResource<Handle>::VulkanResource(Handle handle, VulkanDevice* device_)
        : handle(handle), device(device_), debug_name("")
    {
    }

    template <typename Handle>
    VulkanResource<Handle>::VulkanResource(VulkanResource&& other)
        : debug_name(std::move(other.debug_name)),
          device(other.device),
          handle(other.handle)
    {
        other.device = nullptr;
        other.handle = nullptr;
    }

    template <typename Handle>
    VulkanResource<Handle>& VulkanResource<Handle>::operator=(VulkanResource&& other)
    {
        if (this != &other)
        {
            debug_name = std::move(other.debug_name);
            device = other.device;
            handle = other.handle;

            other.device = nullptr;
            other.handle = nullptr;
        }
        return *this;
    }

    template <typename Handle>
    const std::string& VulkanResource<Handle>::GetDebugName() const
    {
        return debug_name;
    }

    template <typename Handle>
    VulkanDevice& VulkanResource<Handle>::GetDevice()
    {
        return *device;
    }

    template <typename Handle>
    VulkanDevice const& VulkanResource<Handle>::GetDevice() const
    {
        return *device;
    }

    template <typename Handle>
    Handle& VulkanResource<Handle>::GetHandle()
    {
        return handle;
    }

    template <typename Handle>
    const Handle& VulkanResource<Handle>::GetHandle() const
    {
        return handle;
    }

    template <typename Handle>
    uint64_t VulkanResource<Handle>::GetHandleU64() const
    {
        using UintHandle = typename std::conditional<sizeof(Handle) == sizeof(uint32_t), uint32_t, uint64_t>::type;
        return static_cast<uint64_t>(*reinterpret_cast<UintHandle const*>(&handle));
    }

    template <typename Handle>
    VkObjectType VulkanResource<Handle>::GetObjectType() const
    {
        return GetVkObjectType<decltype(handle)>();
    }

    template <typename Handle>
    bool VulkanResource<Handle>::HasDevice() const
    {
        return device != nullptr;
    }

    template <typename Handle>
    bool VulkanResource<Handle>::HasHandle() const
    {
        return handle != nullptr;
    }

    template <typename Handle>
    void VulkanResource<Handle>::SetDebugName(const std::string& name)
    {
        debug_name = name;
        if (device && !debug_name.empty())
        {
            GetDevice().get_debug_utils().set_debug_name(GetDevice().GetHandle(), GetObjectType(), GetHandleU64(),
                                                         debug_name.c_str());
        }
    }

    template <typename Handle>
    void VulkanResource<Handle>::SetHandle(Handle hdl)
    {
        handle = hdl;
    }
}
