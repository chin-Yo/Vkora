#pragma once
#include "Framework/Rendering/Subpass.hpp"

namespace vkb
{
    class CompositionSubpass : public vkb::Subpass
    {
    public:
        CompositionSubpass(vkb::RenderContext& render_context, ShaderSource&& vertex_shader,
                           ShaderSource&& fragment_shader);

        virtual ~CompositionSubpass() = default;

        void prepare() override;

        void draw(vkb::CommandBuffer& command_buffer) override;

    private:
        ShaderVariant compositionVariant;
    };
}
