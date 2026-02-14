#include "Rendering/Subpass/ShadowSubpass.hpp"

#include "GlobalContext.hpp"
#include "Core/Math/MathUtils.h"
#include "Engine/InputEvents.hpp"
#include "Engine/SceneGraph/ComponentPool.hpp"
#include "Engine/SceneGraph/Components/Light.hpp"
#include "Engine/SceneGraph/Components/PerspectiveCamera.hpp"
#include "Engine/SceneGraph/Components/SubMesh.hpp"
#include "Framework/Core/CommandBuffer.hpp"
#include "Framework/Core/VulkanDevice.hpp"
#include "World/WorldManager.hpp"

const glm::vec3 faceDirs[6] = {
    {1, 0, 0}, // +X
    {-1, 0, 0}, // -X
    {0, 1, 0}, // +Y
    {0, -1, 0}, // -Y
    {0, 0, 1}, // +Z
    {0, 0, -1} // -Z
};
const glm::vec3 faceUps[6] = {
    {0, 0, -1},
    {0, 0, -1},
    {0, 0, 1},
    {0, 0, -1},
    {0, 1, 0},
    {0, 1, 0}
};


ShadowSubpass::ShadowSubpass(vkb::RenderContext& render_context, vkb::ShaderSource&& vertex_sourse,
                             vkb::ShaderSource&& fragment_sourse, vkb::ShaderSource&& geometry_sourse): Subpass{
                                                                                                            render_context, std::move(vertex_sourse), std::move(fragment_sourse)
                                                                                                        },
                                                                                                        geometry_shader(std::move(geometry_sourse))
{
}

void ShadowSubpass::prepare()
{
}

void ShadowSubpass::draw(vkb::CommandBuffer& command_buffer)
{
    auto* scene = GRuntimeGlobalContext.worldManager->GetActiveWorld();
    if (!scene)
        return;
    auto& device = command_buffer.GetDevice();
    auto& Lights = scene->GetComponentManager()->GetComponentsByClass<scene::Light>();
    auto& render_frame = get_render_context().get_active_frame();
    if (Lights.empty())
        return;
    vkb::MultisampleState multisample_state{};
    multisample_state.rasterization_samples = get_sample_count();
    command_buffer.set_multisample_state(multisample_state);

    vkb::DepthStencilState depth_stencil_state{};
    depth_stencil_state.depth_compare_op = VK_COMPARE_OP_LESS_OR_EQUAL;
    command_buffer.set_depth_stencil_state(depth_stencil_state);
    
    vkb::RasterizationState rasterization_state{};
    rasterization_state.cull_mode = VK_CULL_MODE_NONE;
    command_buffer.set_rasterization_state(rasterization_state);

    auto& vert_shader_module = device.get_resource_cache().request_shader_module(
        VK_SHADER_STAGE_VERTEX_BIT, get_vertex_shader());
    auto& frag_shader_module = device.get_resource_cache().request_shader_module(
        VK_SHADER_STAGE_FRAGMENT_BIT, get_fragment_shader());
    /*auto& geom_shader_module = device.get_resource_cache().request_shader_module(
        VK_SHADER_STAGE_GEOMETRY_BIT, geometry_shader);*/

    std::vector<vkb::ShaderModule*> shader_modules{&vert_shader_module, &frag_shader_module};
    auto& resource_cache = command_buffer.GetDevice().get_resource_cache();
    auto& pipeline_layout = resource_cache.request_pipeline_layout(shader_modules);
    command_buffer.bind_pipeline_layout(pipeline_layout);
    auto& vertex_input_resources = pipeline_layout.get_resources(vkb::ShaderResourceType::Input,
                                                                 VK_SHADER_STAGE_VERTEX_BIT);
    auto& Cascades = GRuntimeGlobalContext.worldManager->GetViewportCamera()->GetCascades(
        CASCADE_COUNT, MathUtils::EulerToDirection(Lights[0].GetOwner()->GetTransform().GetRotationEuler()));
    struct CascadesUBO
    {
        glm::mat4 viewProj[4] = {};
    } ubo;
    for (int i = 0; i < CASCADE_COUNT; i++)
    {
        ubo.viewProj[i] = Cascades[i].viewProjMatrix;
    }
    auto allocation_cascades = render_frame.allocate_buffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                            sizeof(CascadesUBO),
                                                            0);
    allocation_cascades.update(ubo);
    command_buffer.bind_buffer(allocation_cascades.get_buffer(), allocation_cascades.get_offset(),
                               allocation_cascades.get_size(), 0, 0, 0);
    /*struct ShadowMatricesUBO
    {
        glm::mat4 shadowViewProj[3][6] = {};
    } shadowMats;
    struct LightPosUBO
    {
        glm::vec3 lightPositions[3] = {};
    } lightPos;
    int i = 0;
    for (auto& Light : Lights)
    {
        if (i >= 3)
            break;
        lightPos.lightPositions[i] = Light.GetOwner()->GetTransform().GetTranslation();
        float light_near = 0.1f;
        float light_far = Light.get_properties().range;
        glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, light_far, light_near);

        for (int face = 0; face < 6; ++face)
        {
            glm::mat4 view = glm::lookAt(
                lightPos.lightPositions[i],
                lightPos.lightPositions[i] + faceDirs[face],
                faceUps[face]
            );
            shadowMats.shadowViewProj[i][face] = proj * view;
        }
        i++;
    }
    auto allocation_lightPos = render_frame.allocate_buffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                            sizeof(LightPosUBO),
                                                            0);
    allocation_lightPos.update(lightPos);
    command_buffer.bind_buffer(allocation_lightPos.get_buffer(), allocation_lightPos.get_offset(),
                               allocation_lightPos.get_size(), 0, 1, 0);
    auto allocation_shadowMats = render_frame.allocate_buffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                              sizeof(ShadowMatricesUBO),
                                                              0);
    allocation_shadowMats.update(shadowMats);
    command_buffer.bind_buffer(allocation_shadowMats.get_buffer(), allocation_shadowMats.get_offset(),
                               allocation_shadowMats.get_size(), 0, 2, 0);*/
    auto& Meshes = scene->GetComponentManager()->GetComponentsByClass<scene::SubMesh>();
    for (auto& mesh : Meshes)
    {
        auto& transform = mesh.GetOwner()->GetTransform();
        glm::mat4 modelMatrix = transform.GetWorldMatrix();
        command_buffer.push_constants(vkb::to_bytes(modelMatrix));
        // VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE;
        draw_submesh(command_buffer, mesh, vertex_input_resources, VK_FRONT_FACE_CLOCKWISE);
    }
}

void ShadowSubpass::draw_submesh(vkb::CommandBuffer& command_buffer, scene::SubMesh& sub_mesh,
                                 const std::vector<vkb::ShaderResource>& vertex_input_resources, VkFrontFace front_face)
{
    vkb::VertexInputState vertex_input_state;

    for (auto& input_resource : vertex_input_resources)
    {
        scene::MeshData::VertexAttribute attribute;

        if (!sub_mesh.GetAttribute(input_resource.name, attribute))
        {
            continue;
        }

        VkVertexInputAttributeDescription vertex_attribute{};
        vertex_attribute.binding = 0;
        vertex_attribute.format = attribute.format;
        vertex_attribute.location = input_resource.location;
        vertex_attribute.offset = attribute.offset;

        vertex_input_state.attributes.push_back(vertex_attribute);
    }
    VkVertexInputBindingDescription vertex_binding{};
    vertex_binding.binding = 0;
    vertex_binding.stride = sub_mesh.meshData->vertex_buffer_bindings["Vertex"].stride;

    vertex_input_state.bindings.push_back(vertex_binding);
    command_buffer.set_vertex_input_state(vertex_input_state);

    std::vector<std::reference_wrapper<const vkb::Buffer>> buffers;
    buffers.emplace_back(std::ref(*sub_mesh.meshData->vertex_buffer_bindings["Vertex"].buffer));

    // Bind vertex buffers only for the attribute locations defined
    command_buffer.bind_vertex_buffers(0, std::move(buffers), {0});

    draw_submesh_command(command_buffer, sub_mesh, 4);
}

void ShadowSubpass::draw_submesh_command(vkb::CommandBuffer& command_buffer, scene::SubMesh& sub_mesh,
                                         uint32_t instance_num)
{
    // Draw submesh indexed if indices exists
    if (sub_mesh.meshData->index_count != 0)
    {
        // Bind index buffer of submesh
        command_buffer.bind_index_buffer(*sub_mesh.meshData->index_buffer, sub_mesh.meshData->index_buffer_offset,
                                         sub_mesh.meshData->index_type);

        // Draw submesh using indexed data
        command_buffer.draw_indexed(sub_mesh.meshData->index_count, instance_num, 0, 0, 0);
    }
    else
    {
        // Draw submesh using vertices only
        command_buffer.draw(sub_mesh.meshData->vertices_count, instance_num, 0, 0);
    }
}
