#pragma once
#include "Framework/Rendering/Subpass.hpp"

#define CASCADE_COUNT 4
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
                      const std::vector<vkb::ShaderResource>& vertex_input_resources,
                      VkFrontFace front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE);

    virtual void draw_submesh_command(vkb::CommandBuffer& command_buffer, scene::SubMesh& sub_mesh,
                                      uint32_t instance_num = 1);
};
