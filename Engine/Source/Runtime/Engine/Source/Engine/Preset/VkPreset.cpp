#include "Engine/Preset/VkPreset.hpp"

#include "Framework/Core/Buffer.hpp"
#include "Framework/Core/Queue.hpp"
#include "Framework/Core/VulkanDevice.hpp"
#include "Logging/Logger.hpp"

ps::Texture ps::load_texture(vkb::VulkanDevice& device, const std::string& file, scene::Image::ContentType content_type)
{
    Texture texture{};

    texture.image = scene::Image::load(file, file, content_type);
    texture.image->create_vk_image(device);

    const auto& queue = device.get_queue_by_flags(VK_QUEUE_GRAPHICS_BIT, 0);

    VkCommandBuffer command_buffer = device.create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

    vkb::Buffer stage_buffer = vkb::Buffer::create_staging_buffer(device, texture.image->get_data());

    // Setup buffer copy regions for each mip level
    std::vector<VkBufferImageCopy> bufferCopyRegions;

    auto& mipmaps = texture.image->get_mipmaps();

    for (size_t i = 0; i < mipmaps.size(); i++)
    {
        VkBufferImageCopy buffer_copy_region = {};
        buffer_copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        buffer_copy_region.imageSubresource.mipLevel = vkb::to_u32(i);
        buffer_copy_region.imageSubresource.baseArrayLayer = 0;
        buffer_copy_region.imageSubresource.layerCount = 1;
        buffer_copy_region.imageExtent.width = texture.image->get_extent().width >> i;
        buffer_copy_region.imageExtent.height = texture.image->get_extent().height >> i;
        buffer_copy_region.imageExtent.depth = 1;
        buffer_copy_region.bufferOffset = mipmaps[i].offset;

        bufferCopyRegions.push_back(buffer_copy_region);
    }

    VkImageSubresourceRange subresource_range = {};
    subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresource_range.baseMipLevel = 0;
    subresource_range.levelCount = vkb::to_u32(mipmaps.size());
    subresource_range.layerCount = 1;

    // Image barrier for optimal image (target)
    // Optimal image will be used as destination for the copy
    vkb::image_layout_transition(command_buffer,
                                 texture.image->get_vk_image().GetHandle(),
                                 VK_IMAGE_LAYOUT_UNDEFINED,
                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                 subresource_range);

    // Copy mip levels from staging buffer
    vkCmdCopyBufferToImage(
        command_buffer,
        stage_buffer.GetHandle(),
        texture.image->get_vk_image().GetHandle(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        static_cast<uint32_t>(bufferCopyRegions.size()),
        bufferCopyRegions.data());

    // Change texture image layout to shader read after all mip levels have been copied
    vkb::image_layout_transition(command_buffer,
                                 texture.image->get_vk_image().GetHandle(),
                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                 subresource_range);

    device.flush_command_buffer(command_buffer, queue.get_handle());

    // Calculate valid filter and mipmap modes
    VkFilter filter = VK_FILTER_LINEAR;
    VkSamplerMipmapMode mipmap_mode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    vkb::make_filters_valid(device.get_gpu().get_handle(), texture.image->get_format(), &filter, &mipmap_mode);

    // Create a defaultsampler
    VkSamplerCreateInfo sampler_create_info = {};
    sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_create_info.magFilter = filter;
    sampler_create_info.minFilter = filter;
    sampler_create_info.mipmapMode = mipmap_mode;
    sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create_info.mipLodBias = 0.0f;
    sampler_create_info.compareOp = VK_COMPARE_OP_NEVER;
    sampler_create_info.minLod = 0.0f;
    // Max level-of-detail should match mip level count
    sampler_create_info.maxLod = static_cast<float>(mipmaps.size());
    // Only enable anisotropic filtering if enabled on the device
    // Note that for simplicity, we will always be using max. available anisotropy level for the current device
    // This may have an impact on performance, esp. on lower-specced devices
    // In a real-world scenario the level of anisotropy should be a user setting or e.g. lowered for mobile devices by default
    sampler_create_info.maxAnisotropy = device.get_gpu().get_requested_features().samplerAnisotropy
                                            ? (device.get_gpu().get_properties().limits.maxSamplerAnisotropy)
                                            : 1.0f;
    sampler_create_info.anisotropyEnable = device.get_gpu().get_requested_features().samplerAnisotropy;
    sampler_create_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    VK_CHECK_RESULT(vkCreateSampler(device.GetHandle(), &sampler_create_info, nullptr, &texture.sampler));

    return texture;
}
