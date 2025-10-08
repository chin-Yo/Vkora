#pragma once

#include <cstdio>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include "VkError.hpp"

#include <volk.h>
#include <type_traits>
#include <vk_mem_alloc.h>

// #include "Framework/Common/VkHelpers.hpp"
// #include "Framework/Common/Optional.hpp"
// #include "Framework/Common/VkCommon.hpp

template <typename T>
constexpr VkObjectType GetVkObjectType();

#define DEFINE_VK_OBJECT_TYPE(Type, ObjectType) \
    template <>                                 \
    constexpr VkObjectType GetVkObjectType<Type>() { return ObjectType; }

DEFINE_VK_OBJECT_TYPE(VkInstance, VK_OBJECT_TYPE_INSTANCE)
DEFINE_VK_OBJECT_TYPE(VkPhysicalDevice, VK_OBJECT_TYPE_PHYSICAL_DEVICE)
DEFINE_VK_OBJECT_TYPE(VkDevice, VK_OBJECT_TYPE_DEVICE)
DEFINE_VK_OBJECT_TYPE(VkQueue, VK_OBJECT_TYPE_QUEUE)
DEFINE_VK_OBJECT_TYPE(VkSemaphore, VK_OBJECT_TYPE_SEMAPHORE)
DEFINE_VK_OBJECT_TYPE(VkCommandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER)
DEFINE_VK_OBJECT_TYPE(VkFence, VK_OBJECT_TYPE_FENCE)
DEFINE_VK_OBJECT_TYPE(VkDeviceMemory, VK_OBJECT_TYPE_DEVICE_MEMORY)
DEFINE_VK_OBJECT_TYPE(VkBuffer, VK_OBJECT_TYPE_BUFFER)
DEFINE_VK_OBJECT_TYPE(VkImage, VK_OBJECT_TYPE_IMAGE)
DEFINE_VK_OBJECT_TYPE(VkEvent, VK_OBJECT_TYPE_EVENT)
DEFINE_VK_OBJECT_TYPE(VkQueryPool, VK_OBJECT_TYPE_QUERY_POOL)
DEFINE_VK_OBJECT_TYPE(VkBufferView, VK_OBJECT_TYPE_BUFFER_VIEW)
DEFINE_VK_OBJECT_TYPE(VkImageView, VK_OBJECT_TYPE_IMAGE_VIEW)
DEFINE_VK_OBJECT_TYPE(VkShaderModule, VK_OBJECT_TYPE_SHADER_MODULE)
DEFINE_VK_OBJECT_TYPE(VkPipelineCache, VK_OBJECT_TYPE_PIPELINE_CACHE)
DEFINE_VK_OBJECT_TYPE(VkPipelineLayout, VK_OBJECT_TYPE_PIPELINE_LAYOUT)
DEFINE_VK_OBJECT_TYPE(VkRenderPass, VK_OBJECT_TYPE_RENDER_PASS)
DEFINE_VK_OBJECT_TYPE(VkPipeline, VK_OBJECT_TYPE_PIPELINE)
DEFINE_VK_OBJECT_TYPE(VkDescriptorSetLayout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT)
DEFINE_VK_OBJECT_TYPE(VkSampler, VK_OBJECT_TYPE_SAMPLER)
DEFINE_VK_OBJECT_TYPE(VkDescriptorPool, VK_OBJECT_TYPE_DESCRIPTOR_POOL)
DEFINE_VK_OBJECT_TYPE(VkDescriptorSet, VK_OBJECT_TYPE_DESCRIPTOR_SET)
DEFINE_VK_OBJECT_TYPE(VkFramebuffer, VK_OBJECT_TYPE_FRAMEBUFFER)
DEFINE_VK_OBJECT_TYPE(VkCommandPool, VK_OBJECT_TYPE_COMMAND_POOL)

#define VK_FLAGS_NONE 0 // Custom define for better code readability

#define DEFAULT_FENCE_TIMEOUT 100000000000 // Default fence timeout in nanoseconds

template <class T>
using ShaderStageMap = std::map<VkShaderStageFlagBits, T>;

template <class T>
using BindingMap = std::map<uint32_t, std::map<uint32_t, T>>;

namespace vkb
{
    enum class CommandBufferResetMode
    {
        ResetPool,
        ResetIndividually,
        AlwaysAllocate,
    };

    /**
     * @brief Helper function to determine if a Vulkan format is depth only.
     * @param format Vulkan format to check.
     * @return True if format is a depth only, false otherwise.
     */
    bool is_depth_only_format(VkFormat format);

    /**
     * @brief Helper function to determine if a Vulkan format is depth with stencil.
     * @param format Vulkan format to check.
     * @return True if format is a depth with stencil, false otherwise.
     */
    bool is_depth_stencil_format(VkFormat format);

    /**
     * @brief Helper function to determine if a Vulkan format is depth.
     * @param format Vulkan format to check.
     * @return True if format is a depth, false otherwise.
     */
    bool is_depth_format(VkFormat format);

    /**
     * @brief Helper function to determine a suitable supported depth format based on a priority list
     * @param physical_device The physical device to check the depth formats against
     * @param depth_only (Optional) Wether to include the stencil component in the format or not
     * @param depth_format_priority_list (Optional) The list of depth formats to prefer over one another
     *		  By default we start with the highest precision packed format
     * @return The valid suited depth format
     */
    VkFormat get_suitable_depth_format(VkPhysicalDevice physical_device,
                                       bool depth_only = false,
                                       const std::vector<VkFormat>& depth_format_priority_list = {
                                           VK_FORMAT_D32_SFLOAT,
                                           VK_FORMAT_D24_UNORM_S8_UINT,
                                           VK_FORMAT_D16_UNORM
                                       });

    /**
     * @brief Helper function to pick a blendable format from a priority ordered list
     * @param physical_device The physical device to check the formats against
     * @param format_priority_list List of formats in order of priority
     * @return The selected format
     */
    VkFormat choose_blendable_format(VkPhysicalDevice physical_device,
                                     const std::vector<VkFormat>& format_priority_list);

    /**
     * @brief Helper function to check support for linear filtering and adjust its parameters if required
     * @param physical_device The physical device to check the depth formats against
     * @param format The format to check against
     * @param filter The preferred filter to adjust
     * @param mipmapMode (Optional) The preferred mipmap mode to adjust
     */
    void make_filters_valid(VkPhysicalDevice physical_device, VkFormat format, VkFilter* filter,
                            VkSamplerMipmapMode* mipmapMode = nullptr);

    /**
     * @brief Helper function to determine if a Vulkan descriptor type is a dynamic storage buffer or dynamic uniform buffer.
     * @param descriptor_type Vulkan descriptor type to check.
     * @return True if type is dynamic buffer, false otherwise.
     */
    bool is_dynamic_buffer_descriptor_type(VkDescriptorType descriptor_type);

    /**
     * @brief Helper function to determine if a Vulkan descriptor type is a buffer (either uniform or storage buffer, dynamic or not).
     * @param descriptor_type Vulkan descriptor type to check.
     * @return True if type is buffer, false otherwise.
     */
    bool is_buffer_descriptor_type(VkDescriptorType descriptor_type);

    /**
     * @brief Helper function to get the bits per pixel of a Vulkan format.
     * @param format Vulkan format to check.
     * @return The bits per pixel of the given format, -1 for invalid formats.
     */
    int32_t get_bits_per_pixel(VkFormat format);

    /**
     * @brief Helper functions for compression controls
     */
    std::vector<VkImageCompressionFixedRateFlagBitsEXT> fixed_rate_compression_flags_to_vector(
        VkImageCompressionFixedRateFlagsEXT flags);

    VkImageCompressionPropertiesEXT query_supported_fixed_rate_compression(
        VkPhysicalDevice gpu, const VkImageCreateInfo& create_info);

    VkImageCompressionPropertiesEXT query_applied_compression(VkDevice device, VkImage image);

    /**
     * @brief Image memory barrier structure used to define
     *        memory access for an image view during command recording.
     */

    struct ImageMemoryBarrier
    {
        VkPipelineStageFlags src_stage_mask{VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT};

        VkPipelineStageFlags dst_stage_mask{VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};

        VkAccessFlags src_access_mask{0};

        VkAccessFlags dst_access_mask{0};

        VkImageLayout old_layout{VK_IMAGE_LAYOUT_UNDEFINED};

        VkImageLayout new_layout{VK_IMAGE_LAYOUT_UNDEFINED};

        uint32_t src_queue_family{VK_QUEUE_FAMILY_IGNORED};

        uint32_t dst_queue_family{VK_QUEUE_FAMILY_IGNORED};
    };

    /**
     * @brief Buffer memory barrier structure used to define
     *        memory access for a buffer during command recording.
     */
    struct BufferMemoryBarrier
    {
        VkPipelineStageFlags src_stage_mask{VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT};

        VkPipelineStageFlags dst_stage_mask{VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};

        VkAccessFlags src_access_mask{0};

        VkAccessFlags dst_access_mask{0};
    };

    /**
     * @brief Put an image memory barrier for a layout transition of an image, using explicitly give transition parameters.
     * @param command_buffer The VkCommandBuffer to record the barrier.
     * @param image The VkImage to transition.
     * @param src_stage_mask The VkPipelineStageFlags to use as source.
     * @param dst_stage_mask The VkPipelineStageFlags to use as destination.
     * @param src_access_mask The VkAccessFlags to use as source.
     * @param dst_access_mask The VkAccessFlags to use as destination.
     * @param old_layout The VkImageLayout to transition from.
     * @param new_layout The VkImageLayout to transition to.
     * @param subresource_range The VkImageSubresourceRange to use with the transition.
     */
    void image_layout_transition(VkCommandBuffer command_buffer,
                                 VkImage image,
                                 VkPipelineStageFlags src_stage_mask,
                                 VkPipelineStageFlags dst_stage_mask,
                                 VkAccessFlags src_access_mask,
                                 VkAccessFlags dst_access_mask,
                                 VkImageLayout old_layout,
                                 VkImageLayout new_layout,
                                 VkImageSubresourceRange const& subresource_range);

    /**
     * @brief Put an image memory barrier for a layout transition of an image, on a given subresource range.
     *
     * The src_stage_mask, dst_stage_mask, src_access_mask, and dst_access_mask used are determined from old_layout and new_layout.
     *
     * @param command_buffer The VkCommandBuffer to record the barrier.
     * @param image The VkImage to transition.
     * @param old_layout The VkImageLayout to transition from.
     * @param new_layout The VkImageLayout to transition to.
     * @param subresource_range The VkImageSubresourceRange to use with the transition.
     */
    void image_layout_transition(VkCommandBuffer command_buffer,
                                 VkImage image,
                                 VkImageLayout old_layout,
                                 VkImageLayout new_layout,
                                 VkImageSubresourceRange const& subresource_range);

    /**
     * @brief Put an image memory barrier for a layout transition of an image, on a fixed subresource with first mip level and layer.
     *
     * The src_stage_mask, dst_stage_mask, src_access_mask, and dst_access_mask used are determined from old_layout and new_layout.
     *
     * @param command_buffer The VkCommandBuffer to record the barrier.
     * @param image The VkImage to transition.
     * @param old_layout The VkImageLayout to transition from.
     * @param new_layout The VkImageLayout to transition to.
     */
    void image_layout_transition(VkCommandBuffer command_buffer,
                                 VkImage image,
                                 VkImageLayout old_layout,
                                 VkImageLayout new_layout);

    /**
     * @brief Put an image memory barrier for a layout transition of a vector of images, with a given subresource range per image.
     *
     * The src_stage_mask, dst_stage_mask, src_access_mask, and dst_access_mask used are determined from old_layout and new_layout.
     *
     * @param command_buffer The VkCommandBuffer to record the barrier.
     * @param imagesAndRanges The images to transition, with accompanying subresource ranges.
     * @param old_layout The VkImageLayout to transition from.
     * @param new_layout The VkImageLayout to transition to.
     */
    void image_layout_transition(VkCommandBuffer command_buffer,
                                 std::vector<std::pair<VkImage, VkImageSubresourceRange>> const& imagesAndRanges,
                                 VkImageLayout old_layout,
                                 VkImageLayout new_layout);

    /**
     * @brief Load and store info for a render pass attachment.
     */
    struct LoadStoreInfo
    {
        VkAttachmentLoadOp load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;

        VkAttachmentStoreOp store_op = VK_ATTACHMENT_STORE_OP_STORE;
    };

    namespace gbuffer
    {
        /**
         * @return Load store info to load all and store only the swapchain
         */
        std::vector<LoadStoreInfo> get_load_all_store_swapchain();

        /**
         * @return Load store info to clear all and store only the swapchain
         */
        std::vector<LoadStoreInfo> get_clear_all_store_swapchain();

        /**
         * @return Load store info to clear and store all images
         */
        std::vector<LoadStoreInfo> get_clear_store_all();

        /**
         * @return Default clear values for the G-buffer
         */
        std::vector<VkClearValue> get_clear_value();
    } // namespace gbuffer
}
