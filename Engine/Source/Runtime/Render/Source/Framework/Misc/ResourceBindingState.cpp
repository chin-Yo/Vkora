#include "Framework/Misc/ResourceBindingState.hpp"

namespace vkb
{
    // ===================================
    // ResourceBindingState implementation
    // ===================================

    void ResourceBindingState::reset()
    {
        clear_dirty();
        resource_sets.clear();
    }

    bool ResourceBindingState::is_dirty()
    {
        return dirty;
    }

    void ResourceBindingState::clear_dirty()
    {
        dirty = false;
    }

    void ResourceBindingState::clear_dirty(uint32_t set)
    {
        resource_sets[set].clear_dirty();
    }

    void ResourceBindingState::bind_buffer(const vkb::Buffer &buffer, VkDeviceSize offset, VkDeviceSize range, uint32_t set, uint32_t binding, uint32_t array_element)
    {
        resource_sets[set].bind_buffer(buffer, offset, range, binding, array_element);
        dirty = true;
    }

    void ResourceBindingState::bind_image(const vkb::ImageView &image_view, const vkb::Sampler &sampler, uint32_t set, uint32_t binding, uint32_t array_element)
    {
        resource_sets[set].bind_image(image_view, sampler, binding, array_element);
        dirty = true;
    }

    void ResourceBindingState::bind_image(const vkb::ImageView &image_view, uint32_t set, uint32_t binding, uint32_t array_element)
    {
        resource_sets[set].bind_image(image_view, binding, array_element);
        dirty = true;
    }

    void ResourceBindingState::bind_input(const vkb::ImageView &image_view, uint32_t set, uint32_t binding, uint32_t array_element)
    {
        resource_sets[set].bind_input(image_view, binding, array_element);
        dirty = true;
    }

    const std::unordered_map<uint32_t, ResourceSet> &ResourceBindingState::get_resource_sets()
    {
        return resource_sets;
    }

    // ===================================
    // ResourceSet implementation
    // ===================================

    void ResourceSet::reset()
    {
        clear_dirty();
        resource_bindings.clear();
    }

    bool ResourceSet::is_dirty() const
    {
        return dirty;
    }

    void ResourceSet::clear_dirty()
    {
        dirty = false;
    }

    void ResourceSet::clear_dirty(uint32_t binding, uint32_t array_element)
    {
        resource_bindings[binding][array_element].dirty = false;
    }

    void ResourceSet::bind_buffer(const vkb::Buffer &buffer, VkDeviceSize offset, VkDeviceSize range, uint32_t binding, uint32_t array_element)
    {
        auto &resource_info = resource_bindings[binding][array_element];
        resource_info.dirty = true;
        resource_info.buffer = &buffer;
        resource_info.offset = offset;
        resource_info.range = range;

        dirty = true;
    }

    void ResourceSet::bind_image(const vkb::ImageView &image_view, const vkb::Sampler &sampler, uint32_t binding, uint32_t array_element)
    {
        auto &resource_info = resource_bindings[binding][array_element];
        resource_info.dirty = true;
        resource_info.image_view = &image_view;
        resource_info.sampler = &sampler;

        dirty = true;
    }

    void ResourceSet::bind_image(const vkb::ImageView &image_view, uint32_t binding, uint32_t array_element)
    {
        auto &resource_info = resource_bindings[binding][array_element];
        resource_info.dirty = true;
        resource_info.image_view = &image_view;
        resource_info.sampler = nullptr;

        dirty = true;
    }

    void ResourceSet::bind_input(const vkb::ImageView &image_view, const uint32_t binding, const uint32_t array_element)
    {
        auto &resource_info = resource_bindings[binding][array_element];
        resource_info.dirty = true;
        resource_info.image_view = &image_view;

        dirty = true;
    }

    const BindingMap<ResourceInfo> &ResourceSet::get_resource_bindings() const
    {
        return resource_bindings;
    }

} // namespace vkb