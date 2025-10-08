#pragma once

#include "Framework/Common/VkCommon.hpp"

namespace vkb
{
    class Buffer;
    class ImageView;
    class Sampler;

    struct ResourceInfo
    {
        bool dirty{false};

        const vkb::Buffer *buffer{nullptr};

        VkDeviceSize offset{0};

        VkDeviceSize range{0};

        const ImageView *image_view{nullptr};

        const Sampler *sampler{nullptr};
    };

    /**
     * @brief A resource set is a collection of bindings that contain resources bound by the command buffer.
     * ResourceSet and DescriptorSet have a one-to-one correspondence relationship.
     */
    class ResourceSet
    {
    public:
        void reset();

        bool is_dirty() const;

        void clear_dirty();

        void clear_dirty(uint32_t binding, uint32_t array_element);

        void bind_buffer(const vkb::Buffer &buffer, VkDeviceSize offset, VkDeviceSize range, uint32_t binding,
                         uint32_t array_element);

        void bind_image(const vkb::ImageView &image_view, const vkb::Sampler &sampler, uint32_t binding,
                        uint32_t array_element);

        void bind_image(const vkb::ImageView &image_view, uint32_t binding, uint32_t array_element);

        void bind_input(const vkb::ImageView &image_view, uint32_t binding, uint32_t array_element);

        const BindingMap<ResourceInfo> &get_resource_bindings() const;

    private:
        bool dirty{false};

        BindingMap<ResourceInfo> resource_bindings;
    };

    class ResourceBindingState
    {
    public:
        void reset();

        bool is_dirty();

        void clear_dirty();

        void clear_dirty(uint32_t set);

        void bind_buffer(const vkb::Buffer &buffer, VkDeviceSize offset, VkDeviceSize range, uint32_t set,
                         uint32_t binding, uint32_t array_element);

        void bind_image(const vkb::ImageView &image_view, const vkb::Sampler &sampler, uint32_t set, uint32_t binding,
                        uint32_t array_element);

        void bind_image(const vkb::ImageView &image_view, uint32_t set, uint32_t binding, uint32_t array_element);

        void bind_input(const vkb::ImageView &image_view, uint32_t set, uint32_t binding, uint32_t array_element);

        const std::unordered_map<uint32_t, ResourceSet> &get_resource_sets();

    private:
        bool dirty{false};

        std::unordered_map<uint32_t, ResourceSet> resource_sets;
    };
} // namespace vkb
