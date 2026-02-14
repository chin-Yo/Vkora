/* Copyright (c) 2019-2025, Arm Limited and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 the "License";
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include "Rendering/LightingSubpass.hpp"

#include "GlobalContext.hpp"
#include "Engine/Asset/Manager/AssetManager.hpp"
#include "Engine/SceneGraph/ComponentPool.hpp"
#include "Framework/Core/CommandBuffer.hpp"
#include "Framework/Core/VulkanDevice.hpp"
#include "Engine/SceneGraph/Scene.hpp"
#include "Engine/SceneGraph/Components/Camera.hpp"
#include "Engine/SceneGraph/Components/Light.hpp"
#include "Engine/SceneGraph/Components/PerspectiveCamera.hpp"
#include "Engine/SceneGraph/Components/Skybox.hpp"
#include "Engine/Texture/Texture2D.hpp"
#include "Engine/Texture/TextureCube.hpp"
#include "Framework/Core/Sampler.hpp"
#include "Rendering/RenderSystem.hpp"
#include "Rendering/Subpass/ShadowSubpass.hpp"
#include "Tools/Utils.hpp"
#include "VkPreset/VpSampler.hpp"
#include "World/WorldManager.hpp"

namespace vkb
{
    LightingSubpass::LightingSubpass(RenderContext& render_context, ShaderSource&& vertex_shader,
                                     ShaderSource&& fragment_shader, scene::Camera& cam, scene::Scene& scene_,
                                     std::vector<std::unique_ptr<vkb::RenderTarget>>& viewport_render_targets) :
        Subpass{render_context, std::move(vertex_shader), std::move(fragment_shader)},
        camera{cam},
        scene{scene_}, ViewportRTs{viewport_render_targets}
    {
    }

    void LightingSubpass::prepare()
    {
        // Build all shaders upfront
        auto& resource_cache = get_render_context().get_device().get_resource_cache();
        resource_cache.request_shader_module(VK_SHADER_STAGE_VERTEX_BIT, get_vertex_shader(), lighting_variant);
        resource_cache.request_shader_module(VK_SHADER_STAGE_FRAGMENT_BIT, get_fragment_shader(), lighting_variant);
        ShadowSampler = std::make_unique<vkb::Sampler>(GRuntimeGlobalContext.renderSystem->GetDevice(), vp::ShadowMapSamplerPreset{}.CreateInfo());
    }

    void LightingSubpass::draw(vkb::CommandBuffer& command_buffer)
    {
        vkb::LightingState lighting_state;
        vkb::allocate_lightState(scene.GetComponentManager()->GetComponentsByClass<scene::Light>(),
                                 MAX_DEFERRED_LIGHT_COUNT, lighting_state);

        allocate_lights<DeferredLights>(lighting_state);
        command_buffer.bind_lighting(get_lighting_state(), 1, 1);

        // Get shaders from cache
        auto& resource_cache = command_buffer.GetDevice().get_resource_cache();
        auto& vert_shader_module = resource_cache.request_shader_module(VK_SHADER_STAGE_VERTEX_BIT, get_vertex_shader(),
                                                                        lighting_variant);
        auto& frag_shader_module = resource_cache.request_shader_module(VK_SHADER_STAGE_FRAGMENT_BIT,
                                                                        get_fragment_shader(), lighting_variant);

        std::vector<ShaderModule*> shader_modules{&vert_shader_module, &frag_shader_module};

        // Create pipeline layout and bind it
        auto& pipeline_layout = resource_cache.request_pipeline_layout(shader_modules);
        command_buffer.bind_pipeline_layout(pipeline_layout);

        // we know, that the lighting subpass does not have any vertex stage input -> reset the vertex input state
        assert(pipeline_layout.get_resources(ShaderResourceType::Input, VK_SHADER_STAGE_VERTEX_BIT).empty());
        command_buffer.set_vertex_input_state({});

        // Get image views of the attachments
        auto& render_target = *ViewportRTs[get_render_context().get_active_frame_index()];
        auto& target_views = render_target.get_views();
        assert(3 < target_views.size());

        // Bind depth, albedo, and normal as input attachments
        auto& depth_view = target_views[1];
        command_buffer.bind_input(depth_view, 0, 0, 0);

        auto& albedo_view = target_views[2];
        command_buffer.bind_input(albedo_view, 0, 1, 0);

        auto& normal_view = target_views[3];
        command_buffer.bind_input(normal_view, 0, 2, 0);

        auto& material_view = target_views[4];
        command_buffer.bind_input(material_view, 0, 3, 0);

        auto& Skybox = scene.GetComponentManager()->GetComponentsByClass<scene::Skybox>();
        if (!Skybox.empty() && Skybox[0].EnvCube != nullptr)
        {
            auto SkyEnvCube = Skybox[0].EnvCube;
            command_buffer.bind_image(SkyEnvCube->get_vk_image_view(), *SkyEnvCube->sampler.lock(), 1, 2, 0);
            if (Skybox[0].IrradianceMap != nullptr && Skybox[0].bIsIrradianceMapReady == true)
            {
                auto& Irr = Skybox[0].IrradianceMap;
                command_buffer.bind_image(Irr->get_vk_image_view(), *Irr->sampler.lock(), 1, 3, 0);
            }
            else
            {
                auto* cube = GRuntimeGlobalContext.assetManager->GetTextureCube(DEFAULT_IrradianceMap);
                command_buffer.bind_image(cube->get_vk_image_view(), *cube->sampler.lock(), 1, 3, 0);
            }

            if (Skybox[0].SpecularIBLPrefilter != nullptr && Skybox[0].bIsSpecularIBLPrefilterReady == true)
            {
                auto& Per = Skybox[0].SpecularIBLPrefilter;
                command_buffer.bind_image(Per->get_vk_image_view(), *Per->sampler.lock(), 1, 5, 0);
            }
            else
            {
                auto* PrefilterMap = GRuntimeGlobalContext.assetManager->GetTextureCube(DEFAULT_PrefilterMap);
                command_buffer.bind_image(PrefilterMap->get_vk_image_view(), *PrefilterMap->sampler.lock(), 1, 5, 0);
            }

            if (Skybox[0].BRDFLUT != nullptr && Skybox[0].bIsBRDFLUTReady == true)
            {
                auto& BRDFLUT = Skybox[0].BRDFLUT;
                command_buffer.bind_image(BRDFLUT->get_vk_image_view(), *BRDFLUT->sampler.lock(), 1, 4, 0);
            }
            else
            {
                auto* BRDFLUT = GRuntimeGlobalContext.assetManager->GetTexture(DEFAULT_BRDFLUT);
                command_buffer.bind_image(BRDFLUT->get_vk_image_view(), *BRDFLUT->sampler.lock(), 1, 4, 0);
            }
        }
        else
        {
            auto* cube = GRuntimeGlobalContext.assetManager->GetTextureCube(DEFAULT_IrradianceMap);
            command_buffer.bind_image(cube->get_vk_image_view(), *cube->sampler.lock(), 1, 2, 0);
            command_buffer.bind_image(cube->get_vk_image_view(), *cube->sampler.lock(), 1, 3, 0);
            auto* BRDFLUT = GRuntimeGlobalContext.assetManager->GetTexture(DEFAULT_BRDFLUT);
            command_buffer.bind_image(BRDFLUT->get_vk_image_view(), *BRDFLUT->sampler.lock(), 1, 4, 0);
            auto* PrefilterMap = GRuntimeGlobalContext.assetManager->GetTextureCube(DEFAULT_PrefilterMap);
            command_buffer.bind_image(PrefilterMap->get_vk_image_view(), *PrefilterMap->sampler.lock(), 1, 5, 0);
        }

        command_buffer.bind_image(ShadowRenderTarget->get_views()[0], *ShadowSampler.get(), 2, 0, 0);

        auto& Lights = scene.GetComponentManager()->GetComponentsByClass<scene::Light>();
        auto& render_frame = get_render_context().get_active_frame();
        struct ShadowUniforms
        {
            glm::mat4 view; // 需要视图矩阵来计算 ViewSpace Z 以选择级联
            glm::mat4 cascade_view_proj[CASCADE_COUNT];
            glm::vec4 cascade_splits; // .x, .y, .z, .w (存储分段距离/深度)
            int enable_pcf = 1;
            int debug_cascade; // 1 to show cascade colors
            float _pad1;
            float _pad2;
        } shadow_ubo;
        if (!Lights.empty())
        {
            auto* CameraPtr = GRuntimeGlobalContext.worldManager->GetViewportCamera();
            assert(CameraPtr && "The camera is ineffective.");
            shadow_ubo.view = CameraPtr->GetViewMatrix();
            for (int i = 0; i < CASCADE_COUNT; i++)
            {
                auto& Cascades = CameraPtr->GetCascades();
                shadow_ubo.cascade_view_proj[i] = Cascades[i].viewProjMatrix;
                shadow_ubo.cascade_splits[i] = Cascades[i].splitDepth;
            }
        }
        auto allocation_ShadowUniforms = render_frame.allocate_buffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(ShadowUniforms),
                                                                      0);
        allocation_ShadowUniforms.update(shadow_ubo);
        command_buffer.bind_buffer(allocation_ShadowUniforms.get_buffer(), allocation_ShadowUniforms.get_offset(),
                                   allocation_ShadowUniforms.get_size(), 2, 1, 0);
        // Set cull mode to front as full screen triangle is clock-wise
        RasterizationState rasterization_state;
        rasterization_state.cull_mode = VK_CULL_MODE_FRONT_BIT;
        command_buffer.set_rasterization_state(rasterization_state);

        // Populate uniform values
        LightUniform light_uniform;

        // Inverse resolution
        light_uniform.inv_resolution.x = 1.0f / render_target.get_extent().width;
        light_uniform.inv_resolution.y = 1.0f / render_target.get_extent().height;

        // Inverse view projection
        light_uniform.inv_view_proj = glm::inverse(
            vkb::vulkan_style_projection(camera.GetProjection()) * camera.GetViewMatrix());

        light_uniform.camPos.rgb = camera.GetOwner()->GetTransform().GetTranslation();

        // Allocate a buffer using the buffer pool from the active frame to store uniform values and bind it

        auto allocation = render_frame.allocate_buffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(LightUniform));
        allocation.update(light_uniform);
        command_buffer.bind_buffer(allocation.get_buffer(), allocation.get_offset(), allocation.get_size(), 1, 0, 0);

        // Draw full screen triangle triangle
        command_buffer.draw(3, 1, 0, 0);
    }
} // namespace vkb
