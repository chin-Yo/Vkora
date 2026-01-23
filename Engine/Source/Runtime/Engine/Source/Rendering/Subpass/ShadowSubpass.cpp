#include "Rendering/Subpass/ShadowSubpass.hpp"

#include "GlobalContext.hpp"
#include "Engine/InputEvents.hpp"
#include "Engine/SceneGraph/ComponentPool.hpp"
#include "Engine/SceneGraph/Components/Light.hpp"
#include "Engine/SceneGraph/Components/SubMesh.hpp"
#include "Framework/Core/CommandBuffer.hpp"
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
    auto& Lights = scene->GetComponentManager()->GetComponentsByClass<scene::Light>();
    auto& render_frame = get_render_context().get_active_frame();

    struct ShadowMatricesUBO
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
        glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, Light.get_properties().range);

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
                               allocation_shadowMats.get_size(), 0, 2, 0);
    auto& Meshes = scene->GetComponentManager()->GetComponentsByClass<scene::SubMesh>();
    for (auto& mesh : Meshes)
    {
        auto& transform = mesh.GetOwner()->GetTransform();
        auto allocation = render_frame.allocate_buffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(glm::mat4),
                                                       0);
        glm::mat4 modelMatrix = transform.GetWorldMatrix();
        allocation.update(modelMatrix);
        command_buffer.bind_buffer(allocation.get_buffer(), allocation.get_offset(), allocation.get_size(), 0, 0, 0);

        // VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE;
        draw_submesh(command_buffer, mesh, VK_FRONT_FACE_CLOCKWISE);
    }
}

void ShadowSubpass::draw_submesh(vkb::CommandBuffer& command_buffer, scene::SubMesh& sub_mesh, VkFrontFace front_face)
{
}
