#pragma once

#include "Framework/Common/VkHelpers.hpp"
#include "Framework/Common/VkCommon.hpp"
#include "Framework/Core/VulkanResource.hpp"
#include "Framework/Misc/ResourceCache.hpp"
#include "Framework/Core/PhysicalDevice.hpp"

namespace vkb
{
    struct DriverVersion
    {
        uint16_t major;
        uint16_t minor;
        uint16_t patch;
    };

    class PhysicalDevice;
    class Queue;
    class CommandPool;
    class FencePool;
    class CommandBuffer;
    class DebugUtils;
    class Buffer;

    class VulkanDevice : public vkb::VulkanResource<VkDevice>
    {
    public:
        /**
         * @brief VulkanDevice constructor
         * @param gpu A valid Vulkan physical device and the requested gpu features
         * @param surface The surface
         * @param debug_utils The debug utils to be associated to this device
         * @param requested_extensions (Optional) List of required device extensions and whether support is optional or not
         */
        VulkanDevice(PhysicalDevice& gpu,
                     VkSurfaceKHR surface,
                     std::unique_ptr<DebugUtils>&& debug_utils,
                     std::unordered_map<const char*, bool> requested_extensions = {});

        /**
         * @brief VulkanDevice constructor
         * @param gpu A valid Vulkan physical device and the requested gpu features
         * @param vulkan_device A valid Vulkan device
         * @param surface The surface
         */
        VulkanDevice(PhysicalDevice& gpu,
                     VkDevice& vulkan_device,
                     VkSurfaceKHR surface);

        VulkanDevice(const VulkanDevice&) = delete;

        VulkanDevice(VulkanDevice&&) = delete;

        ~VulkanDevice();

        VulkanDevice& operator=(const VulkanDevice&) = delete;

        VulkanDevice& operator=(VulkanDevice&&) = delete;

        const PhysicalDevice& get_gpu() const;

        /**
         * @brief Returns the debug utils associated with this VulkanDevice.
         */
        inline const DebugUtils& get_debug_utils() const
        {
            return *debug_utils;
        }

        /**
         * @return The version of the driver of the current physical device
         */
        DriverVersion get_driver_version() const;

        /**
         * @return Whether an image format is supported by the GPU
         */
        bool is_image_format_supported(VkFormat format) const;

        const Queue& get_queue(uint32_t queue_family_index, uint32_t queue_index);

        const Queue& get_queue_by_flags(VkQueueFlags queue_flags, uint32_t queue_index) const;

        const Queue& get_queue_by_present(uint32_t queue_index) const;

        /**
         * @brief Manually adds a new queue from a given family index to this device
         * @param global_index Index at where the queue should be placed inside the already existing list of queues
         * @param family_index Index of the queue family from which the queue will be created
         * @param properties Vulkan queue family properties
         * @param can_present True if the queue is able to present images
         */
        void add_queue(size_t global_index, uint32_t family_index, VkQueueFamilyProperties properties,
                       VkBool32 can_present);

        /**
         * @brief Finds a suitable graphics queue to submit to
         * @return The first present supported queue, otherwise just any graphics queue
         */
        const Queue& get_suitable_graphics_queue() const;

        bool is_extension_supported(const std::string& extension) const;

        bool is_enabled(const char* extension) const;

        uint32_t get_queue_family_index(VkQueueFlagBits queue_flag);

        uint32_t get_num_queues_for_queue_family(uint32_t queue_family_index);

        vkb::CommandPool& get_command_pool() const;

        /**
         * @brief Checks that a given memory type is supported by the GPU
         * @param bits The memory requirement type bits
         * @param properties The memory property to search for
         * @param memory_type_found True if found, false if not found
         * @returns The memory type index of the found memory type
         */
        uint32_t get_memory_type(uint32_t bits, VkMemoryPropertyFlags properties,
                                 VkBool32* memory_type_found = nullptr) const;

        /**
         * @brief Copies a buffer from one to another
         * @param src The buffer to copy from
         * @param dst The buffer to copy to
         * @param queue The queue to submit the copy command to
         * @param copy_region The amount to copy, if null copies the entire buffer
         */
        void copy_buffer(vkb::Buffer& src, vkb::Buffer& dst, VkQueue queue, VkBufferCopy* copy_region = nullptr);

        /**
         * @brief Creates a command pool
         * @param queue_index The queue index this command pool is associated with
         * @param flags The command pool flags
         * @returns A valid VkCommandPool
         */
        VkCommandPool create_command_pool(uint32_t queue_index, VkCommandPoolCreateFlags flags = 0);

        /**
         * @brief Requests a command buffer from the device's command pool
         * @param level The command buffer level
         * @param begin Whether the command buffer should be implicitly started before it's returned
         * @returns A valid VkCommandBuffer
         */
        VkCommandBuffer create_command_buffer(VkCommandBufferLevel level, bool begin = false) const;

        /**
         * @brief Submits and frees up a given command buffer
         * @param command_buffer The command buffer
         * @param queue The queue to submit the work to
         * @param free Whether the command buffer should be implicitly freed up
         * @param signalSemaphore An optional semaphore to signal when the commands have been executed
         */
        void flush_command_buffer(VkCommandBuffer command_buffer, VkQueue queue, bool free = true,
                                  VkSemaphore signalSemaphore = VK_NULL_HANDLE) const;

        /**
         * @brief Requests a command buffer from the general command_pool
         * @return A new command buffer
         */
        std::shared_ptr<vkb::CommandBuffer> request_command_buffer() const;

        FencePool& get_fence_pool() const;

        /**
         * @brief Creates the fence pool used by this device
         */
        void create_internal_fence_pool();

        /**
         * @brief Creates the command pool used by this device
         */
        void create_internal_command_pool();

        /**
         * @brief Creates and sets up the Vulkan memory allocator
         */
        void prepare_memory_allocator();

        /**
         * @brief Requests a fence to the fence pool
         * @return A vulkan fence
         */
        VkFence request_fence() const;

        VkResult wait_idle() const;

        ResourceCache& get_resource_cache();

    private:
        const PhysicalDevice& gpu;

        VkSurfaceKHR surface{VK_NULL_HANDLE};

        std::unique_ptr<DebugUtils> debug_utils;

        std::vector<const char*> enabled_extensions{};

        std::vector<std::vector<Queue>> queues;

        /// A command pool associated to the primary queue
        std::unique_ptr<vkb::CommandPool> command_pool;

        /// A fence pool associated to the primary queue
        std::unique_ptr<FencePool> fence_pool;

        ResourceCache resource_cache;
    };
} // namespace vkb
