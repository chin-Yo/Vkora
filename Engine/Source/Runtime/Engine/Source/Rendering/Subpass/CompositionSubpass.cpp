#include "Rendering/Subpass/CompositionSubpass.hpp"

#include "Framework/Core/CommandBuffer.hpp"
#include "Framework/Core/VulkanDevice.hpp"


namespace vkb
{
    CompositionSubpass::CompositionSubpass(vkb::RenderContext& render_context, ShaderSource&& vertex_shader,
                                           ShaderSource&& fragment_shader) :
        Subpass{render_context, std::move(vertex_shader), std::move(fragment_shader)}
    {
    }

    void CompositionSubpass::prepare()
    {
    }

    void CompositionSubpass::draw(vkb::CommandBuffer& command_buffer)
    {
        // Get shaders from cache
        auto& resource_cache = command_buffer.GetDevice().get_resource_cache();
        auto& vert_shader_module = resource_cache.request_shader_module(VK_SHADER_STAGE_VERTEX_BIT, get_vertex_shader(),
                                                                        compositionVariant);
        auto& frag_shader_module = resource_cache.request_shader_module(VK_SHADER_STAGE_FRAGMENT_BIT,
                                                                        get_fragment_shader(), compositionVariant);
        std::vector<ShaderModule*> shader_modules{&vert_shader_module, &frag_shader_module};

        // Create pipeline layout and bind it
        auto& pipeline_layout = resource_cache.request_pipeline_layout(shader_modules);
        command_buffer.bind_pipeline_layout(pipeline_layout);

        // we know, that the composition subpass does not have any vertex stage input -> reset the vertex input state
        assert(pipeline_layout.get_resources(ShaderResourceType::Input, VK_SHADER_STAGE_VERTEX_BIT).empty());
        command_buffer.set_vertex_input_state({});
    }
}
