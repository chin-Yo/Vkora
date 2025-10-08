#pragma once
#include <vector>
#include <volk.h>
#include <set>
#include "GLFW/glfw3.h"

namespace vkb
{
    struct SwapchainProperties
    {
        VkSwapchainKHR old_swapchain;
        uint32_t image_count{3};
        VkExtent2D extent{};
        VkSurfaceFormatKHR surface_format{};
        uint32_t array_layers;
        VkImageUsageFlags image_usage;
        VkSurfaceTransformFlagBitsKHR pre_transform;
        VkCompositeAlphaFlagBitsKHR composite_alpha;
        VkPresentModeKHR present_mode;
    };

    class VulkanDevice;

    class Swapchain
    {
    public:
        /**
         * @brief Constructor to create a swapchain by changing the extent
         *        only and preserving the configuration from the old swapchain.
         */
        Swapchain(Swapchain& old_swapchain, const VkExtent2D& extent);

        /**
         * @brief Constructor to create a swapchain by changing the image count
         *        only and preserving the configuration from the old swapchain.
         */
        Swapchain(Swapchain& old_swapchain, const uint32_t image_count);

        /**
         * @brief Constructor to create a swapchain by changing the image usage
         *        only and preserving the configuration from the old swapchain.
         */
        Swapchain(Swapchain& old_swapchain, const std::set<VkImageUsageFlagBits>& image_usage_flags);

        /**
         * @brief Constructor to create a swapchain by changing the extent
         *        and transform only and preserving the configuration from the old swapchain.
         */
        Swapchain(Swapchain& swapchain, const VkExtent2D& extent, const VkSurfaceTransformFlagBitsKHR transform);

        /**
         * @brief Constructor to create a swapchain by changing the compression settings
         *        only and preserving the configuration from the old swapchain.
         */
        Swapchain(Swapchain& swapchain, const VkImageCompressionFlagsEXT requested_compression,
                  const VkImageCompressionFixedRateFlagsEXT requested_compression_fixed_rate);

        /**
         * @brief Constructor to create a swapchain.
         */
        Swapchain(VulkanDevice& device,
                  VkSurfaceKHR surface,
                  const VkPresentModeKHR present_mode,
                  const std::vector<VkPresentModeKHR>& present_mode_priority_list = {
                      VK_PRESENT_MODE_FIFO_KHR,
                      VK_PRESENT_MODE_MAILBOX_KHR
                  },
                  const std::vector<VkSurfaceFormatKHR>& surface_format_priority_list = {
                      {VK_FORMAT_R8G8B8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
                      {VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}
                  },
                  const VkExtent2D& extent = {},
                  const uint32_t image_count = 3,
                  const VkSurfaceTransformFlagBitsKHR transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
                  const std::set<VkImageUsageFlagBits>& image_usage_flags = {
                      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_USAGE_TRANSFER_SRC_BIT
                  },
                  const VkImageCompressionFlagsEXT requested_compression = VK_IMAGE_COMPRESSION_DEFAULT_EXT,
                  const VkImageCompressionFixedRateFlagsEXT requested_compression_fixed_rate =
                      VK_IMAGE_COMPRESSION_FIXED_RATE_NONE_EXT);

        /**
         * @brief Constructor to create a swapchain from the old swapchain
         *        by configuring all parameters.
         */
        Swapchain(Swapchain& old_swapchain,
                  VulkanDevice& device,
                  VkSurfaceKHR surface,
                  const VkPresentModeKHR present_mode,
                  const std::vector<VkPresentModeKHR>& present_mode_priority_list = {
                      VK_PRESENT_MODE_FIFO_KHR, VK_PRESENT_MODE_MAILBOX_KHR
                  },
                  const std::vector<VkSurfaceFormatKHR>& surface_format_priority_list = {
                      {VK_FORMAT_R8G8B8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
                      {VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}
                  },
                  const VkExtent2D& extent = {},
                  const uint32_t image_count = 3,
                  const VkSurfaceTransformFlagBitsKHR transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
                  const std::set<VkImageUsageFlagBits>& image_usage_flags = {
                      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_USAGE_TRANSFER_SRC_BIT
                  },
                  const VkImageCompressionFlagsEXT requested_compression = VK_IMAGE_COMPRESSION_DEFAULT_EXT,
                  const VkImageCompressionFixedRateFlagsEXT requested_compression_fixed_rate =
                      VK_IMAGE_COMPRESSION_FIXED_RATE_NONE_EXT);

        Swapchain(const Swapchain&) = delete;

        Swapchain(Swapchain&& other);

        ~Swapchain();

        Swapchain& operator=(const Swapchain&) = delete;

        Swapchain& operator=(Swapchain&&) = delete;

        bool is_valid() const;

        VulkanDevice& get_device();

        VkSwapchainKHR get_handle() const;

        VkResult acquire_next_image(uint32_t& image_index, VkSemaphore image_acquired_semaphore,
                                    VkFence fence = VK_NULL_HANDLE) const;

        const VkExtent2D& get_extent() const;

        VkFormat get_format() const;

        VkSurfaceFormatKHR get_surface_format() const;

        const std::vector<VkImage>& get_images() const;

        VkSurfaceTransformFlagBitsKHR get_transform() const;

        VkSurfaceKHR get_surface() const;

        VkImageUsageFlags get_usage() const;

        VkPresentModeKHR get_present_mode() const;

        VkImageCompressionFlagsEXT get_applied_compression() const;

        /**
         * Helper functions for compression controls
         */

        struct SurfaceFormatCompression
        {
            VkSurfaceFormat2KHR surface_format{VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR};
            VkImageCompressionPropertiesEXT compression_properties{VK_STRUCTURE_TYPE_IMAGE_COMPRESSION_PROPERTIES_EXT};
        };

        static std::vector<SurfaceFormatCompression> query_supported_fixed_rate_compression(
            VulkanDevice& device, const VkSurfaceKHR& surface);

    private:
        VulkanDevice& device;

        VkSurfaceKHR surface{VK_NULL_HANDLE};

        VkSwapchainKHR handle{VK_NULL_HANDLE};

        std::vector<VkImage> images;

        SwapchainProperties properties;

        // A list of present modes in order of priority (vector[0] has high priority, vector[size-1] has low priority)
        std::vector<VkPresentModeKHR> present_mode_priority_list = {
            VK_PRESENT_MODE_FIFO_KHR,
            VK_PRESENT_MODE_MAILBOX_KHR
        };

        // A list of surface formats in order of priority (vector[0] has high priority, vector[size-1] has low priority)
        std::vector<VkSurfaceFormatKHR> surface_format_priority_list = {
            {VK_FORMAT_R8G8B8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
            {VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}
        };

        std::set<VkImageUsageFlagBits> image_usage_flags;

        VkImageCompressionFlagsEXT requested_compression{VK_IMAGE_COMPRESSION_DEFAULT_EXT};

        VkImageCompressionFixedRateFlagsEXT requested_compression_fixed_rate{VK_IMAGE_COMPRESSION_FIXED_RATE_NONE_EXT};
    };
}
