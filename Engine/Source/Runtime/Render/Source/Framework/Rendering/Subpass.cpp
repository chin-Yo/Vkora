#include "Framework/Rendering/Subpass.hpp"
#include "Framework/Rendering/RenderTarget.hpp"

namespace vkb
{

    Subpass::Subpass(vkb::RenderContext &render_context, ShaderSource &&vertex_source, ShaderSource &&fragment_source)
        : render_context{render_context},
          vertex_shader{std::move(vertex_source)},
          fragment_shader{std::move(fragment_source)}
    {
    }

    const std::vector<uint32_t> &Subpass::get_input_attachments() const
    {
        return input_attachments;
    }

    LightingState &Subpass::get_lighting_state()
    {
        return lighting_state;
    }

    const std::vector<uint32_t> &Subpass::get_output_attachments() const
    {
        return output_attachments;
    }

    typename vkb::RenderContext &Subpass::get_render_context()
    {
        return render_context;
    }

    std::unordered_map<std::string, ShaderResourceMode> const &Subpass::get_resource_mode_map() const
    {
        return resource_mode_map;
    }

    typename VkSampleCountFlagBits Subpass::get_sample_count() const
    {
        return sample_count;
    }

    const ShaderSource &Subpass::get_vertex_shader() const
    {
        return vertex_shader;
    }

    const std::vector<uint32_t> &Subpass::get_color_resolve_attachments() const
    {
        return color_resolve_attachments;
    }

    const std::string &Subpass::get_debug_name() const
    {
        return debug_name;
    }

    const uint32_t &Subpass::get_depth_stencil_resolve_attachment() const
    {
        return depth_stencil_resolve_attachment;
    }

    typename VkResolveModeFlagBits Subpass::get_depth_stencil_resolve_mode() const
    {
        return depth_stencil_resolve_mode;
    }

    typename vkb::DepthStencilState &Subpass::get_depth_stencil_state()
    {
        return depth_stencil_state;
    }

    const bool &Subpass::get_disable_depth_stencil_attachment() const
    {
        return disable_depth_stencil_attachment;
    }

    const ShaderSource &Subpass::get_fragment_shader() const
    {
        return fragment_shader;
    }

    void Subpass::set_color_resolve_attachments(std::vector<uint32_t> const &color_resolve)
    {
        color_resolve_attachments = color_resolve;
    }

    void Subpass::set_depth_stencil_resolve_attachment(uint32_t depth_stencil_resolve)
    {
        depth_stencil_resolve_attachment = depth_stencil_resolve;
    }

    void Subpass::set_debug_name(const std::string &name)
    {
        debug_name = name;
    }

    void Subpass::set_disable_depth_stencil_attachment(bool disable_depth_stencil)
    {
        disable_depth_stencil_attachment = disable_depth_stencil;
    }

    void Subpass::set_depth_stencil_resolve_mode(VkResolveModeFlagBits mode)
    {
        depth_stencil_resolve_mode = mode;
    }

    void Subpass::set_input_attachments(std::vector<uint32_t> const &input)
    {
        input_attachments = input;
    }

    void Subpass::set_output_attachments(std::vector<uint32_t> const &output)
    {
        output_attachments = output;
    }

    void Subpass::set_sample_count(VkSampleCountFlagBits sample_count_)
    {
        sample_count = sample_count_;
    }

    void Subpass::update_render_target_attachments(RenderTarget &render_target)
    {
        render_target.set_input_attachments(input_attachments);
        render_target.set_output_attachments(output_attachments);
    }
}