#include "Rendering/RenderSystem.hpp"

#include "GlobalContext.hpp"
#include "backends/imgui_impl_vulkan.h"
#include "Engine/Asset/Manager/AssetManager.hpp"
#include "Engine/SceneGraph/Components/PerspectiveCamera.hpp"
#include "Engine/Texture/Texture2D.hpp"
#include "Framework/Core/CommandBuffer.hpp"
#include "Framework/Core/Queue.hpp"
#include "Framework/Core/Sampler.hpp"
#include "Framework/Platform/Window.hpp"
#include "Framework/Rendering/RenderFrame.hpp"
#include "Framework/Rendering/Subpass.hpp"
#include "Misc/Paths.hpp"
#include "Rendering/GeometrySubpass.hpp"
#include "Rendering/LightingSubpass.hpp"
#include "Tools/Utils.hpp"
#include "World/WorldManager.hpp"
#include "UIManage/EditorUIManager.hpp"

RenderSystem::RenderSystem(vkb::Window* window, vkb::VulkanDevice* device, VkSurfaceKHR surface)
    : window(window), device(device), surface(surface)
{
}

RenderSystem::~RenderSystem()
{
    Finish();
    render_pipeline.reset();
    EditorUIRenderpass.reset();
    ViewportRTs.clear();
    render_context.reset();
}

bool RenderSystem::Prepare()
{
    CreateRenderContext();
    render_context->prepare(1, vkb::RenderTarget::ONE_IMAGE_FUNC);
    std::set<VkImageUsageFlagBits> usage = {VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT};
    GetRenderContext().update_swapchain(usage);
    return true;
}

bool RenderSystem::RenderPrepare()
{
    render_pipeline = CreateOneRenderpassTwoSubpasses(*GRuntimeGlobalContext.worldManager->GetActiveWorld()
                                                      , *GRuntimeGlobalContext.worldManager->GetViewportCamera());
    return true;
}

void RenderSystem::RequestGpuFeatures(vkb::PhysicalDevice& gpu)
{
    // To be overridden by sample
}

void RenderSystem::Draw(vkb::CommandBuffer& command_buffer, vkb::RenderTarget& render_target)
{
    auto& views = render_target.get_views();

    {
        // Image 0 is the swapchain
        vkb::ImageMemoryBarrier memory_barrier{};
        memory_barrier.old_layout = VK_IMAGE_LAYOUT_UNDEFINED;
        memory_barrier.new_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        memory_barrier.src_access_mask = 0;
        memory_barrier.dst_access_mask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        memory_barrier.src_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        memory_barrier.dst_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        command_buffer.image_memory_barrier(views[0], memory_barrier);
        render_target.set_layout(0, memory_barrier.new_layout);
    }

    // draw_renderpass is a virtual function, thus we have to call that, instead of directly calling draw_renderpass_impl!
    DrawRenderpass(command_buffer, render_target);

    {
        vkb::ImageMemoryBarrier memory_barrier{};
        memory_barrier.old_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        memory_barrier.new_layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        memory_barrier.src_access_mask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        memory_barrier.src_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        memory_barrier.dst_stage_mask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

        command_buffer.image_memory_barrier(views[0], memory_barrier);
        render_target.set_layout(0, memory_barrier.new_layout);
    }
}

void RenderSystem::Render(vkb::CommandBuffer& command_buffer)
{
    if (render_pipeline)
    {
        render_pipeline->draw(command_buffer, render_context->get_active_frame().get_render_target());
    }
}

void RenderSystem::DrawRenderpass(vkb::CommandBuffer& command_buffer, vkb::RenderTarget& render_target)
{
    if (OffScreenResourcesReady)
    {
        auto& RT = *ViewportRTs[GetRenderContext().get_active_frame_index()];
        auto& views = RT.get_views();
        {
            // Image 0 is the swapchain
            vkb::ImageMemoryBarrier memory_barrier{};
            memory_barrier.old_layout = VK_IMAGE_LAYOUT_UNDEFINED;
            memory_barrier.new_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            memory_barrier.src_access_mask = {};
            memory_barrier.dst_access_mask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            memory_barrier.src_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            memory_barrier.dst_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

            command_buffer.image_memory_barrier(views[0], memory_barrier);
            RT.set_layout(0, memory_barrier.new_layout);

            // Skip 1 as it is handled later as a depth-stencil attachment
            for (size_t i = 2; i < views.size(); ++i)
            {
                command_buffer.image_memory_barrier(views[i], memory_barrier);
                RT.set_layout(static_cast<uint32_t>(i), memory_barrier.new_layout);
            }
        }

        {
            vkb::ImageMemoryBarrier memory_barrier{};
            memory_barrier.old_layout = VK_IMAGE_LAYOUT_UNDEFINED;
            memory_barrier.new_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            memory_barrier.src_access_mask = {};
            memory_barrier.dst_access_mask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            memory_barrier.src_stage_mask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            memory_barrier.dst_stage_mask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

            command_buffer.image_memory_barrier(views[1], memory_barrier);
            RT.set_layout(1, memory_barrier.new_layout);
        }

        DrawPipeline(command_buffer, RT, *render_pipeline);

        {
            vkb::ImageMemoryBarrier memory_barrier{};
            memory_barrier.old_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            memory_barrier.new_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            memory_barrier.src_access_mask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            memory_barrier.dst_access_mask = VK_ACCESS_SHADER_READ_BIT;
            memory_barrier.src_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            memory_barrier.dst_stage_mask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

            command_buffer.image_memory_barrier(views[0], memory_barrier);
            RT.set_layout(0, memory_barrier.new_layout);
        }
    }

    SetViewportAndScissor(command_buffer, render_target.get_extent());
    VkClearValue clearValues[1];
    clearValues[0].color = {{1.0f, 0.0f, 0.0f, 1.0f}};
    VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
    renderPassBeginInfo.renderPass = EditorUIRenderpass->GetHandle();
    renderPassBeginInfo.renderArea.offset.x = 0;
    renderPassBeginInfo.renderArea.offset.y = 0;
    renderPassBeginInfo.renderArea.extent.width = render_target.get_extent().width;
    renderPassBeginInfo.renderArea.extent.height = render_target.get_extent().height;
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = clearValues;
    auto& framebuffer = device->get_resource_cache().request_framebuffer(render_target, *EditorUIRenderpass);
    renderPassBeginInfo.framebuffer = framebuffer.get_handle();
    vkCmdBeginRenderPass(command_buffer.GetHandle(), &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), command_buffer.GetHandle());
    vkCmdEndRenderPass(command_buffer.GetHandle());
}

void RenderSystem::Update(float delta_time)
{
    // update_gui(delta_time);
    auto command_buffer = render_context->begin();
    UIManager->BeginFrame();
    UIManager->RenderUI();
    UIManager->EndFrame();
    // Collect the performance data for the sample graphs
    // update_stats(delta_time);

    command_buffer->begin(VkCommandBufferUsageFlagBits::VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    // stats->begin_sampling(*command_buffer);

    Draw(*command_buffer, render_context->get_active_frame().get_render_target());

    // stats->end_sampling(*command_buffer);
    command_buffer->end();

    render_context->submit(command_buffer);
}

void RenderSystem::UpdateDebugWindow()
{
}

void RenderSystem::Finish()
{
    if (device)
    {
        vkDeviceWaitIdle(device->GetHandle());
    }
}

void RenderSystem::SetViewportAndScissor(vkb::CommandBuffer const& command_buffer, VkExtent2D const& extent)
{
    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(command_buffer.GetHandle(), 0, 1, &viewport);

    VkRect2D scissor;
    scissor.offset = {0, 0};
    scissor.extent = extent;
    vkCmdSetScissor(command_buffer.GetHandle(), 0, 1, &scissor);
}

void RenderSystem::CreateRenderContext()
{
    CreateRenderContext_Impl(surface_priority_list);
}

void RenderSystem::CreateRenderContext_Impl(const std::vector<VkSurfaceFormatKHR>& surface_priority_list)
{
    VkPresentModeKHR present_mode = (window->GetProperties().vsync == vkb::Window::Vsync::ON)
                                        ? VK_PRESENT_MODE_FIFO_KHR
                                        : VK_PRESENT_MODE_MAILBOX_KHR;
    std::vector<VkPresentModeKHR> present_mode_priority_list{
        VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR
    };
    render_context =
        std::make_unique<vkb::RenderContext>(*device, surface, *window, present_mode,
                                             present_mode_priority_list, surface_priority_list);
}

void RenderSystem::ResetStatsView()
{
}

bool RenderSystem::Resize(uint32_t width, uint32_t height)
{
    return false;
}

void RenderSystem::SetRenderContext(std::unique_ptr<vkb::RenderContext>&& rc)
{
    render_context.reset(rc.release());
}

void RenderSystem::SetRenderPipeline(std::unique_ptr<vkb::RenderPipeline>&& rp)
{
    render_pipeline.reset(rp.release());
}

void RenderSystem::InitializeUIRenderBackend(EditorUIInterface* UIManager)
{
    this->UIManager = UIManager;
    // TODO
    /*EditorUI->OnViewportChange.append([this](const ImVec2& Size)
    {
        this->ViewportResize(Size);
    });*/

    auto pRenderPass = vks::RenderPassBuilder(device->GetHandle())
                       .addAttachment(
                           render_context->get_format(), VK_SAMPLE_COUNT_1_BIT,
                           VK_ATTACHMENT_LOAD_OP_CLEAR,
                           VK_ATTACHMENT_STORE_OP_STORE,
                           VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                           VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) // Type conversions are all explicit.
                       .addSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS, {0})
                       .addDependency(VK_SUBPASS_EXTERNAL, 0,
                                      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)
                       .buildRaw();

    EditorUIRenderpass = std::make_unique<vkb::RenderPass>(*device, pRenderPass);

    this->UIManager->Prepare(EditorUIRenderpass->GetHandle(), device->get_suitable_graphics_queue().get_handle(),
                             render_context->get_swapchain().get_images().size(),
                             render_context->get_swapchain().get_images().size());
}

std::unique_ptr<vkb::RenderPipeline> RenderSystem::CreateOneRenderpassTwoSubpasses(
    scene::Scene& scene, scene::Camera& camera)
{
    // Geometry subpass
    auto geometry_vs = vkb::ShaderSource{Paths::GetShaderFullPath("deferred/geometry.vert.spv")};
    auto geometry_fs = vkb::ShaderSource{Paths::GetShaderFullPath("deferred/geometry.frag.spv")};
    auto scene_subpass = std::make_unique<vkb::GeometrySubpass>(GetRenderContext(), std::move(geometry_vs),
                                                                std::move(geometry_fs), scene, camera);

    // Outputs are depth, albedo, and normal
    scene_subpass->set_output_attachments({1, 2, 3, 4});
    scene_subpass->defaultTexture = GRuntimeGlobalContext.assetManager->GetTexture();
    // Lighting subpass
    auto lighting_vs = vkb::ShaderSource{Paths::GetShaderFullPath("deferred/lighting.vert.spv")};
    auto lighting_fs = vkb::ShaderSource{Paths::GetShaderFullPath("deferred/Pbr_lighting.frag.spv")};
    auto lighting_subpass = std::make_unique<vkb::LightingSubpass>(GetRenderContext(), std::move(lighting_vs),
                                                                   std::move(lighting_fs), camera, scene,
                                                                   ViewportRTs);

    // Inputs are depth, albedo, and normal from the geometry subpass
    lighting_subpass->set_input_attachments({1, 2, 3, 4});

    // Create subpasses pipeline
    std::vector<std::unique_ptr<vkb::Subpass>> subpasses{};
    subpasses.push_back(std::move(scene_subpass));
    subpasses.push_back(std::move(lighting_subpass));

    auto tmp_render_pipeline = std::make_unique<vkb::RenderPipeline>(std::move(subpasses));

    tmp_render_pipeline->set_load_store(vkb::gbuffer::get_clear_all_store_swapchain());

    tmp_render_pipeline->set_clear_value(vkb::gbuffer::get_clear_value());

    return tmp_render_pipeline;
}

std::unique_ptr<vkb::RenderTarget> RenderSystem::CreateRenderTarget(ImVec2 size)
{
    VkExtent3D extent = {static_cast<uint32_t>(size.x), static_cast<uint32_t>(size.y), 1};
    // G-Buffer should fit 128-bit budget for buffer color storage
    // in order to enable subpasses merging by the driver
    // Light (swapchain_image) RGBA8_UNORM   (32-bit)
    // Albedo                  RGBA8_UNORM   (32-bit)
    // Normal                  RGB10A2_UNORM (32-bit)
    vkb::Image viewport_image{
        GetDevice(),
        extent,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY
    };

    vkb::Image depth_image{
        GetDevice(),
        extent,
        vkb::get_suitable_depth_format(GetDevice().get_gpu().get_handle()),
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | rt_usage_flags,
        VMA_MEMORY_USAGE_GPU_ONLY
    };

    vkb::Image albedo_image{
        GetDevice(),
        extent,
        albedo_format,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | rt_usage_flags,
        VMA_MEMORY_USAGE_GPU_ONLY
    };

    vkb::Image normal_image{
        GetDevice(),
        extent,
        normal_format,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | rt_usage_flags,
        VMA_MEMORY_USAGE_GPU_ONLY
    };

    vkb::Image material_image{
        GetDevice(),
        extent,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | rt_usage_flags,
        VMA_MEMORY_USAGE_GPU_ONLY
    };

    std::vector<vkb::Image> images;

    // Attachment 0
    images.push_back(std::move(viewport_image));

    // Attachment 1
    images.push_back(std::move(depth_image));

    // Attachment 2
    images.push_back(std::move(albedo_image));

    // Attachment 3
    images.push_back(std::move(normal_image));

    // Attachment 4
    images.push_back(std::move(material_image));

    return std::make_unique<vkb::RenderTarget>(std::move(images));
}

void RenderSystem::ResetViewportRTs(ImVec2& size, vkb::Sampler* sampler,
                                    std::vector<VkDescriptorSet>& ViewportDescriptorSets)
{
    if (size.x < 256 && size.y < 256)
    {
        return;
    }
    Finish();
    ViewportRTs.clear();
    OffScreenResourcesReady = false;

    ViewportRTs.resize(GetRenderContext().get_render_frames().size());
    for (uint32_t i = 0; i < ViewportRTs.size(); i++)
    {
        ViewportRTs[i] = CreateRenderTarget(size);
    }

    for (uint32_t i = 0; i < ViewportDescriptorSets.size(); i++)
    {
        if (ViewportDescriptorSets[i])
            ImGui_ImplVulkan_RemoveTexture(ViewportDescriptorSets[i]);

        ViewportDescriptorSets[i]
            = ImGui_ImplVulkan_AddTexture(sampler->GetHandle(),
                                          ViewportRTs[i]->get_views()[0].GetHandle(),
                                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
    auto* camera = GRuntimeGlobalContext.worldManager->GetViewportCamera();
    camera->SetAspectRatio(size.x / size.y);
    OffScreenResourcesReady = true;
}

void RenderSystem::DrawPipeline(vkb::CommandBuffer& command_buffer, vkb::RenderTarget& render_target,
                                vkb::RenderPipeline& render_pipeline)
{
    auto& extent = render_target.get_extent();

    VkViewport viewport{};
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    command_buffer.set_viewport(0, {viewport});

    VkRect2D scissor{};
    scissor.extent = extent;
    command_buffer.set_scissor(0, {scissor});

    render_pipeline.draw(command_buffer, render_target);

    command_buffer.end_render_pass();
}

std::vector<VkSurfaceFormatKHR> const& RenderSystem::GetSurfacePriorityList() const
{
    return surface_priority_list;
}

std::vector<VkSurfaceFormatKHR>& RenderSystem::GetSurfacePriorityList()
{
    return surface_priority_list;
}
