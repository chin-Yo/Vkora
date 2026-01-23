#pragma once
#include "Framework/Rendering/Subpass.hpp"


namespace scene
{
    class SubMesh;
}

class ShadowSubpass : public vkb::Subpass
{
public:
    ShadowSubpass(vkb::RenderContext& render_context, vkb::ShaderSource&& vertex_sourse,
                  vkb::ShaderSource&& fragment_sourse, vkb::ShaderSource&& geometry_sourse);

    void prepare() override;

    void draw(vkb::CommandBuffer& command_buffer) override;

protected:
    vkb::ShaderSource geometry_shader;

    void draw_submesh(vkb::CommandBuffer& command_buffer, scene::SubMesh& sub_mesh,
                      VkFrontFace front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE);
};
