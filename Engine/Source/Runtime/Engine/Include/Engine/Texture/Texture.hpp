#pragma once

#include <memory>
#include <string>
#include <vector>
#include <volk.h>

#include "Framework/Core/ImageView.hpp"

namespace vkb
{
    class Image;
    class VulkanDevice;
}

/**
* @brief Create a texture. The default memory type is OnlyGPU.
*/
class Texture
{
public:
    /**
    * @brief Mipmap information
    */
    struct Mipmap
    {
        /// Mipmap level
        uint32_t level = 0;

        /// Byte offset used for uploading
        VkDeviceSize offset = 0;

        /// Width depth and height of the mipmap
        VkExtent3D extent = {0, 0, 0};
    };

    /**
     * @brief Type of content held in image.
     * This helps to steer the image loaders when deciding what the format should be.
     * Some image containers don't know whether the data they contain is sRGB or not.
     * Since most applications save color images in sRGB, knowing that an image
     * contains color data helps us to better guess its format when unknown.
     */
    enum ContentType
    {
        Unknown,
        Color,
        Other
    };

    //Texture(const std::string& name, std::vector<uint8_t>&& data = {}, std::vector<Mipmap>&& mipmaps = {{}});

    //static std::unique_ptr<Texture> load(const std::string& name, const std::string& uri, ContentType content_type);

    virtual ~Texture() = default;

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&&) = default;
    Texture& operator=(Texture&&) = default;

    const std::vector<uint8_t>& get_data() const;

    void clear_data();

    VkFormat get_format() const;

    const VkExtent3D& get_extent() const;

    const uint32_t get_layers() const;

    const std::vector<Mipmap>& get_mipmaps() const;

    const std::vector<std::vector<VkDeviceSize>>& get_offsets() const;

    void generate_mipmaps();

    void create_vk_image(vkb::VulkanDevice& device, VkImageUsageFlags extra_usage = 0);

    const vkb::Image& get_vk_image() const;

    const vkb::ImageView& get_vk_image_view() const;

    void coerce_format_to_srgb();

    std::string get_name() const;

    virtual void CopyDataToGPU(vkb::VulkanDevice& device);

protected:
    Texture(const std::string& name, VkImageViewType type, VkImageCreateFlags flags,
            std::vector<uint8_t>&& data = {}, std::vector<Mipmap>&& mipmaps = {});

    std::vector<uint8_t>& get_mut_data();

    void set_data(const uint8_t* raw_data, size_t size);

    void set_format(VkFormat format);

    void set_width(uint32_t width);

    void set_height(uint32_t height);

    void set_depth(uint32_t depth);

    void set_layers(uint32_t layers);

    void set_offsets(const std::vector<std::vector<VkDeviceSize>>& offsets);

    Mipmap& get_mipmap(size_t index);

    std::vector<Mipmap>& get_mut_mipmaps();

    std::string name;

    std::vector<uint8_t> data;

    VkFormat format{VK_FORMAT_R8G8B8A8_UNORM};

    VkImageLayout layout{VK_IMAGE_LAYOUT_UNDEFINED};

    uint32_t layers{1};

    std::vector<Mipmap> mipmaps{{}}; // default has [0]

    // Offsets stored like offsets[array_layer][mipmap_layer]
    std::vector<std::vector<VkDeviceSize>> offsets;

    std::unique_ptr<vkb::Image> vk_image;

    std::unique_ptr<vkb::ImageView> vk_image_view;

    VkImageViewType view_type;
    VkImageCreateFlags image_create_flags{0};
};
