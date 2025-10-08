#pragma once

#include <mutex>

#include "ResourceRecord.hpp"
#include "ResourceReplay.hpp"
#include "Framework/Core/PipelineLayout.hpp"
#include "Framework/Core/DescriptorSetLayout.hpp"
#include "Framework/Core/DescriptorPool.hpp"
#include "Framework/Core/RenderPass.hpp"
#include "Framework/Core/Pipeline.hpp"
#include "Framework/Core/DescriptorSet.hpp"
#include "Framework/Core/Framebuffer.hpp"


namespace vkb
{
	class ImageView;
	class VulkanDevice;
    /**
     * @brief Struct to hold the internal state of the Resource Cache
     *
     */
    struct ResourceCacheState
    {
        std::unordered_map<std::size_t, ShaderModule> shader_modules;

        std::unordered_map<std::size_t, PipelineLayout> pipeline_layouts;

        std::unordered_map<std::size_t, DescriptorSetLayout> descriptor_set_layouts;

        std::unordered_map<std::size_t, DescriptorPool> descriptor_pools;

        std::unordered_map<std::size_t, RenderPass> render_passes;

        std::unordered_map<std::size_t, GraphicsPipeline> graphics_pipelines;

        std::unordered_map<std::size_t, ComputePipeline> compute_pipelines;

        std::unordered_map<std::size_t, DescriptorSet> descriptor_sets;

        std::unordered_map<std::size_t, Framebuffer> framebuffers;
    };

    class ResourceCache
	{
	public:
		ResourceCache(VulkanDevice &device);

		ResourceCache(const ResourceCache &) = delete;

		ResourceCache(ResourceCache &&) = delete;

		ResourceCache &operator=(const ResourceCache &) = delete;

		ResourceCache &operator=(ResourceCache &&) = delete;

		void warmup(const std::vector<uint8_t> &data);

		std::vector<uint8_t> serialize();

		void set_pipeline_cache(VkPipelineCache pipeline_cache);

		ShaderModule &request_shader_module(VkShaderStageFlagBits stage, const ShaderSource &glsl_source, const ShaderVariant &shader_variant = {});

		PipelineLayout &request_pipeline_layout(const std::vector<ShaderModule *> &shader_modules);

		DescriptorSetLayout &request_descriptor_set_layout(const uint32_t set_index,
														   const std::vector<ShaderModule *> &shader_modules,
														   const std::vector<ShaderResource> &set_resources);

		GraphicsPipeline &request_graphics_pipeline(PipelineState &pipeline_state);

		ComputePipeline &request_compute_pipeline(PipelineState &pipeline_state);

		DescriptorSet &request_descriptor_set(DescriptorSetLayout &descriptor_set_layout,
											  const BindingMap<VkDescriptorBufferInfo> &buffer_infos,
											  const BindingMap<VkDescriptorImageInfo> &image_infos);

		RenderPass &request_render_pass(const std::vector<Attachment> &attachments,
										const std::vector<LoadStoreInfo> &load_store_infos,
										const std::vector<SubpassInfo> &subpasses);

		Framebuffer &request_framebuffer(const RenderTarget &render_target,
										 const RenderPass &render_pass);

		void clear_pipelines();

		/// @brief Update those descriptor sets referring to old views
		/// @param old_views Old image views referred by descriptor sets
		/// @param new_views New image views to be referred
		void update_descriptor_sets(const std::vector<ImageView> &old_views, const std::vector<ImageView> &new_views);

		void clear_framebuffers();

		void clear();

		const ResourceCacheState &get_internal_state() const;

	private:
		VulkanDevice &device;

		ResourceRecord recorder;

		ResourceReplay replayer;

		VkPipelineCache pipeline_cache{VK_NULL_HANDLE};

		ResourceCacheState state;

		std::mutex descriptor_set_mutex;

		std::mutex pipeline_layout_mutex;

		std::mutex shader_module_mutex;

		std::mutex descriptor_set_layout_mutex;

		std::mutex graphics_pipeline_mutex;

		std::mutex render_pass_mutex;

		std::mutex compute_pipeline_mutex;

		std::mutex framebuffer_mutex;
	};
} // namespace vkb
