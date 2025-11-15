#include "Engine/Texture/Texture2D.hpp"

#include <stb_image.h>

#include "Framework/Common/VkHelpers.hpp"
#include "Framework/Core/Buffer.hpp"
#include "Framework/Core/Image.hpp"
#include "Framework/Core/Queue.hpp"
#include "Framework/Core/Sampler.hpp"
#include "Framework/Core/VulkanDevice.hpp"
#include "Logging/Logger.hpp"
#include "Misc/FileLoader.hpp"
#include "VkPreset/VpSampler.hpp"


Texture2D::Texture2D(const std::string& name, const std::string& uri, ContentType content_type)
    : Texture(name)
{
    auto data = FileLoader::ReadFileBinary(uri);

    auto fun = [](const std::string& uri) -> std::string
    {
        auto dot_pos = uri.find_last_of('.');
        if (dot_pos == std::string::npos)
        {
            LOG_WARN("Uri has no extension")
            return "";
        }

        return uri.substr(dot_pos + 1);
    };
    auto extension = fun(uri);

    if (extension == "png" || extension == "jpg")
    {
        int width;
        int height;
        int comp;
        int req_comp = 4;

        auto data_buffer = reinterpret_cast<const stbi_uc*>(data.data());
        auto data_size = static_cast<int>(data.size());

        auto raw_data = stbi_load_from_memory(data_buffer, data_size, &width, &height, &comp, req_comp);

        if (!raw_data)
        {
            LOG_ERROR("Failed to load {} {}", name, stbi_failure_reason())
            return;
        }

        set_data(raw_data, width * height * req_comp);
        stbi_image_free(raw_data);

        set_format(content_type == Color ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM);
        set_width(vkb::to_u32(width));
        set_height(vkb::to_u32(height));
        set_depth(1u);
    }
}

void Texture2D::MoveToGPU(vkb::VulkanDevice& device)
{
    this->create_vk_image(device);

    const auto& queue = device.get_queue_by_flags(VK_QUEUE_GRAPHICS_BIT, 0);

    VkCommandBuffer command_buffer = device.create_command_buffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

    vkb::Buffer stage_buffer = vkb::Buffer::create_staging_buffer(device, this->get_data());

    // Setup buffer copy regions for each mip level
    std::vector<VkBufferImageCopy> bufferCopyRegions;

    auto& mipmaps = this->get_mipmaps();

    for (size_t i = 0; i < mipmaps.size(); i++)
    {
        VkBufferImageCopy buffer_copy_region = {};
        buffer_copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        buffer_copy_region.imageSubresource.mipLevel = vkb::to_u32(i);
        buffer_copy_region.imageSubresource.baseArrayLayer = 0;
        buffer_copy_region.imageSubresource.layerCount = 1;
        buffer_copy_region.imageExtent.width = this->get_extent().width >> i;
        buffer_copy_region.imageExtent.height = this->get_extent().height >> i;
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
                                 this->get_vk_image().GetHandle(),
                                 VK_IMAGE_LAYOUT_UNDEFINED,
                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                 subresource_range);

    // Copy mip levels from staging buffer
    vkCmdCopyBufferToImage(
        command_buffer,
        stage_buffer.GetHandle(),
        this->get_vk_image().GetHandle(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        static_cast<uint32_t>(bufferCopyRegions.size()),
        bufferCopyRegions.data());

    // Change texture image layout to shader read after all mip levels have been copied
    vkb::image_layout_transition(command_buffer,
                                 this->get_vk_image().GetHandle(),
                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                 subresource_range);

    device.flush_command_buffer(command_buffer, queue.get_handle());
}
