#include "Engine/Texture/Texture.hpp"

#include <cassert>
#include <stb_image_resize.h>
#include <vk_mem_alloc.h>

#include "Framework/Common/VkHelpers.hpp"
#include "Framework/Core/Buffer.hpp"
#include "Framework/Core/Image.hpp"
#include "Framework/Core/Queue.hpp"
#include "Misc/FileLoader.hpp"
#include "Framework/Core/VulkanDevice.hpp"

bool is_astc(const VkFormat format)
{
    return (format == VK_FORMAT_ASTC_4x4_UNORM_BLOCK ||
        format == VK_FORMAT_ASTC_4x4_SRGB_BLOCK ||
        format == VK_FORMAT_ASTC_5x4_UNORM_BLOCK ||
        format == VK_FORMAT_ASTC_5x4_SRGB_BLOCK ||
        format == VK_FORMAT_ASTC_5x5_UNORM_BLOCK ||
        format == VK_FORMAT_ASTC_5x5_SRGB_BLOCK ||
        format == VK_FORMAT_ASTC_6x5_UNORM_BLOCK ||
        format == VK_FORMAT_ASTC_6x5_SRGB_BLOCK ||
        format == VK_FORMAT_ASTC_6x6_UNORM_BLOCK ||
        format == VK_FORMAT_ASTC_6x6_SRGB_BLOCK ||
        format == VK_FORMAT_ASTC_8x5_UNORM_BLOCK ||
        format == VK_FORMAT_ASTC_8x5_SRGB_BLOCK ||
        format == VK_FORMAT_ASTC_8x6_UNORM_BLOCK ||
        format == VK_FORMAT_ASTC_8x6_SRGB_BLOCK ||
        format == VK_FORMAT_ASTC_8x8_UNORM_BLOCK ||
        format == VK_FORMAT_ASTC_8x8_SRGB_BLOCK ||
        format == VK_FORMAT_ASTC_10x5_UNORM_BLOCK ||
        format == VK_FORMAT_ASTC_10x5_SRGB_BLOCK ||
        format == VK_FORMAT_ASTC_10x6_UNORM_BLOCK ||
        format == VK_FORMAT_ASTC_10x6_SRGB_BLOCK ||
        format == VK_FORMAT_ASTC_10x8_UNORM_BLOCK ||
        format == VK_FORMAT_ASTC_10x8_SRGB_BLOCK ||
        format == VK_FORMAT_ASTC_10x10_UNORM_BLOCK ||
        format == VK_FORMAT_ASTC_10x10_SRGB_BLOCK ||
        format == VK_FORMAT_ASTC_12x10_UNORM_BLOCK ||
        format == VK_FORMAT_ASTC_12x10_SRGB_BLOCK ||
        format == VK_FORMAT_ASTC_12x12_UNORM_BLOCK ||
        format == VK_FORMAT_ASTC_12x12_SRGB_BLOCK);
}

// When the color-space of a loaded image is unknown (from KTX1 for example) we
// may want to assume that the loaded data is in sRGB format (since it usually is).
// In those cases, this helper will get called which will force an existing unorm
// format to become an srgb format where one exists. If none exist, the format will
// remain unmodified.
static VkFormat maybe_coerce_to_srgb(VkFormat fmt)
{
    switch (fmt)
    {
    case VK_FORMAT_R8_UNORM:
        return VK_FORMAT_R8_SRGB;
    case VK_FORMAT_R8G8_UNORM:
        return VK_FORMAT_R8G8_SRGB;
    case VK_FORMAT_R8G8B8_UNORM:
        return VK_FORMAT_R8G8B8_SRGB;
    case VK_FORMAT_B8G8R8_UNORM:
        return VK_FORMAT_B8G8R8_SRGB;
    case VK_FORMAT_R8G8B8A8_UNORM:
        return VK_FORMAT_R8G8B8A8_SRGB;
    case VK_FORMAT_B8G8R8A8_UNORM:
        return VK_FORMAT_B8G8R8A8_SRGB;
    case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
        return VK_FORMAT_A8B8G8R8_SRGB_PACK32;
    case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
        return VK_FORMAT_BC1_RGB_SRGB_BLOCK;
    case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
        return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
    case VK_FORMAT_BC2_UNORM_BLOCK:
        return VK_FORMAT_BC2_SRGB_BLOCK;
    case VK_FORMAT_BC3_UNORM_BLOCK:
        return VK_FORMAT_BC3_SRGB_BLOCK;
    case VK_FORMAT_BC7_UNORM_BLOCK:
        return VK_FORMAT_BC7_SRGB_BLOCK;
    case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
        return VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK;
    case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
        return VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK;
    case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
        return VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK;
    case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
        return VK_FORMAT_ASTC_4x4_SRGB_BLOCK;
    case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
        return VK_FORMAT_ASTC_5x4_SRGB_BLOCK;
    case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
        return VK_FORMAT_ASTC_5x5_SRGB_BLOCK;
    case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
        return VK_FORMAT_ASTC_6x5_SRGB_BLOCK;
    case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
        return VK_FORMAT_ASTC_6x6_SRGB_BLOCK;
    case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
        return VK_FORMAT_ASTC_8x5_SRGB_BLOCK;
    case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
        return VK_FORMAT_ASTC_8x6_SRGB_BLOCK;
    case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
        return VK_FORMAT_ASTC_8x8_SRGB_BLOCK;
    case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
        return VK_FORMAT_ASTC_10x5_SRGB_BLOCK;
    case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
        return VK_FORMAT_ASTC_10x6_SRGB_BLOCK;
    case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
        return VK_FORMAT_ASTC_10x8_SRGB_BLOCK;
    case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
        return VK_FORMAT_ASTC_10x10_SRGB_BLOCK;
    case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
        return VK_FORMAT_ASTC_12x10_SRGB_BLOCK;
    case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
        return VK_FORMAT_ASTC_12x12_SRGB_BLOCK;
    case VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG:
        return VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG;
    case VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG:
        return VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG;
    case VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG:
        return VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG;
    case VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG:
        return VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG;
    default:
        return fmt;
    }
}

const std::vector<uint8_t>& Texture::get_data() const
{
    return data;
}

void Texture::clear_data()
{
    data.clear();
    data.shrink_to_fit();
}

VkFormat Texture::get_format() const
{
    return format;
}

const VkExtent3D& Texture::get_extent() const
{
    assert(!mipmaps.empty());
    return mipmaps[0].extent;
}

const uint32_t Texture::get_layers() const
{
    return layers;
}

const std::vector<Texture::Mipmap>& Texture::get_mipmaps() const
{
    return mipmaps;
}

const std::vector<std::vector<VkDeviceSize>>& Texture::get_offsets() const
{
    return offsets;
}

void Texture::create_vk_image(vkb::VulkanDevice& device, VkImageUsageFlags extra_usage)
{
    assert(!vk_image && !vk_image_view && "Vulkan image already constructed");

    vk_image = std::make_unique<vkb::Image>(device,
                                            get_extent(),
                                            format,
                                            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | extra_usage,
                                            VMA_MEMORY_USAGE_GPU_ONLY,
                                            VK_SAMPLE_COUNT_1_BIT,
                                            vkb::to_u32(mipmaps.size()),
                                            layers,
                                            VK_IMAGE_TILING_OPTIMAL,
                                            image_create_flags);
    vk_image->SetDebugName(get_name());

    vk_image_view = std::make_unique<vkb::ImageView>(*vk_image, view_type);
    vk_image_view->SetDebugName("View on " + get_name());
}

const vkb::Image& Texture::get_vk_image() const
{
    assert(vk_image && "Vulkan image was not created");
    return *vk_image;
}

const vkb::ImageView& Texture::get_vk_image_view() const
{
    assert(vk_image_view && "Vulkan image view was not created");
    return *vk_image_view;
}

Texture::Mipmap& Texture::get_mipmap(const size_t index)
{
    assert(index < mipmaps.size());
    return mipmaps[index];
}

// Note that this function returns the required size for ALL mip levels, *including* the base level.
uint32_t get_required_mipmaps_size(const VkExtent3D& extent)
{
    constexpr uint32_t channels = 4;
    auto width = std::max<uint32_t>(1, extent.width);
    auto height = std::max<uint32_t>(1, extent.height);
    auto size = width * height * channels;
    auto result = size;
    while (size != channels)
    {
        width = std::max<uint32_t>(1u, width >> 1);
        height = std::max<uint32_t>(1u, height >> 1);
        size = width * height * channels;
        result += size;
    }
    return result;
}

void Texture::generate_mipmaps()
{
    assert(mipmaps.size() == 1 && "Mipmaps already generated");

    if (mipmaps.size() > 1)
    {
        return; // Do not generate again
    }

    auto extent = get_extent();
    auto next_width = std::max<uint32_t>(1u, extent.width / 2);
    auto next_height = std::max<uint32_t>(1u, extent.height / 2);
    auto channels = 4;
    auto next_size = next_width * next_height * channels;

    // Allocate for all the mips at once.  The function returns the total size needed for the
    // existing base mip as well as all the mips that will be generated.
    data.reserve(get_required_mipmaps_size(extent));

    while (true)
    {
        // Make space for next mipmap
        auto old_size = vkb::to_u32(data.size());
        data.resize(old_size + next_size);

        auto& prev_mipmap = mipmaps.back();
        // Update mipmaps
        Mipmap next_mipmap{};
        next_mipmap.level = prev_mipmap.level + 1;
        next_mipmap.offset = old_size;
        next_mipmap.extent = {next_width, next_height, 1u};

        // Fill next mipmap memory
        stbir_resize_uint8(data.data() + prev_mipmap.offset, prev_mipmap.extent.width,
                           prev_mipmap.extent.height, 0,
                           data.data() + next_mipmap.offset, next_mipmap.extent.width,
                           next_mipmap.extent.height, 0, channels);

        mipmaps.emplace_back(std::move(next_mipmap));

        // Next mipmap values
        next_width = std::max<uint32_t>(1u, next_width / 2);
        next_height = std::max<uint32_t>(1u, next_height / 2);
        next_size = next_width * next_height * channels;

        if (next_width == 1 && next_height == 1)
        {
            break;
        }
    }
}

std::vector<Texture::Mipmap>& Texture::get_mut_mipmaps()
{
    return mipmaps;
}

std::vector<uint8_t>& Texture::get_mut_data()
{
    return data;
}

void Texture::set_data(const uint8_t* raw_data, size_t size)
{
    assert(data.empty() && "Image data already set");
    data = {raw_data, raw_data + size};
}

void Texture::set_format(const VkFormat f)
{
    format = f;
}

void Texture::set_width(const uint32_t width)
{
    assert(!mipmaps.empty());
    mipmaps[0].extent.width = width;
}

void Texture::set_height(const uint32_t height)
{
    assert(!mipmaps.empty());
    mipmaps[0].extent.height = height;
}

void Texture::set_depth(const uint32_t depth)
{
    assert(!mipmaps.empty());
    mipmaps[0].extent.depth = depth;
}

void Texture::set_layers(uint32_t l)
{
    layers = l;
}

void Texture::set_offsets(const std::vector<std::vector<VkDeviceSize>>& o)
{
    offsets = o;
}

void Texture::coerce_format_to_srgb()
{
    format = maybe_coerce_to_srgb(format);
}

std::string Texture::get_name() const
{
    return name;
}


void Texture::CopyDataToGPU(vkb::VulkanDevice& device)
{
    if (!vk_image)
    {
        this->create_vk_image(device);
    }

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
    subresource_range.layerCount = layers;

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
    layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

Texture::Texture(const std::string& name, VkImageViewType type, VkImageCreateFlags flags, std::vector<uint8_t>&& data,
                 std::vector<Mipmap>&& mipmaps): name(name), view_type(type), image_create_flags(flags),
                                                 data(std::move(data)), mipmaps(std::move(mipmaps))
{
}
