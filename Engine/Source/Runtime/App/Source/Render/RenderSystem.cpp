#include "Render/RenderSystem.hpp"

#include "backends/imgui_impl_vulkan.h"
#include "Framework/Core/CommandBuffer.hpp"
#include "Framework/Core/Queue.hpp"
#include "Framework/Platform/Window.hpp"
#include "Framework/Rendering/RenderFrame.hpp"
#include "Framework/Rendering/Subpass.hpp"
#include "Import/GLTFLoader.hpp"
#include "Misc/Paths.hpp"
#include "Render/EditorUI.hpp"
#include "Rendering/GeometrySubpass.hpp"
#include "Rendering/LightingSubpass.hpp"
#include "SceneGraph/Components/PerspectiveCamera.h"
#include "SceneGraph/Scene.h"
#include "SceneGraph/Node.h"
#include "SceneGraph/Script.h"
#include "SceneGraph/Scripts/Animation.h"
#include "Tools/Utils.hpp"

RenderSystem::~RenderSystem()
{
    Finish();
    scene.reset();
    EditorUIRenderpass.reset();
    ViewportRTs.clear();
    UIManager->Shutdown();
    render_context.reset();
    device.reset();

    if (surface)
    {
        vkDestroySurfaceKHR(instance->get_handle(), surface, nullptr);
    }

    instance.reset();
}

bool RenderSystem::Prepare(const ApplicationOptions &options)
{
    LOG_INFO("Initializing vulkan render system!")
    assert(options.window != nullptr && "Window is invalid");
    window = options.window;

    // static vk::detail::DynamicLoader dl;
    // VULKAN_HPP_DEFAULT_DISPATCHER.init(dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr"));

    bool headless = window->GetWindowMode() == vkb::Window::Mode::Headless;

    VK_CHECK_RESULT(volkInitialize());

    // Creating the vulkan instance
    for (const char *extension_name : window->GetRequiredSurfaceExtensions())
    {
        AddInstanceExtension(extension_name);
    }

#ifdef DEBUG
    {
        uint32_t available_extension_count = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &available_extension_count, nullptr);
        std::vector<VkExtensionProperties> available_instance_extensions(available_extension_count);
        vkEnumerateInstanceExtensionProperties(nullptr, &available_extension_count,
                                               available_instance_extensions.data());
        auto debugExtensionIt =
            std::find_if(available_instance_extensions.begin(), available_instance_extensions.end(),
                         [](VkExtensionProperties const &ep)
                         {
                             return strcmp(ep.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0;
                         });
        if (debugExtensionIt != available_instance_extensions.end())
        {
            LOGI("Vulkan debug utils enabled ({})", VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

            debug_utils = std::make_unique<vkb::DebugUtilsExtDebugUtils>();
            AddInstanceExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
    }
#endif

    instance = CreateInstance();
    // VULKAN_HPP_DEFAULT_DISPATCHER.init(instance->get_handle());
    surface = window->CreateSurface(*instance);
    if (!surface)
    {
        throw std::runtime_error("Failed to create window surface.");
    }

    auto &gpu = instance->get_suitable_gpu(surface, headless);
    gpu.set_high_priority_graphics_queue_enable(high_priority_graphics_queue);

    if (gpu.get_features().textureCompressionASTC_LDR)
    {
        gpu.get_mutable_requested_features().textureCompressionASTC_LDR = true;
    }

    RequestGpuFeatures(gpu);

    // Creating vulkan device, specifying the swapchain extension always
    // If using VK_EXT_headless_surface, we still create and use a swap-chain
    {
        AddDeviceExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        if (instance_extensions.find(VK_KHR_DISPLAY_EXTENSION_NAME) != instance_extensions.end())
        {
            AddDeviceExtension(VK_KHR_DISPLAY_SWAPCHAIN_EXTENSION_NAME, /*optional=*/true);
        }
    }
    // TODO
#ifdef VK_ENABLE_PORTABILITY
    // VK_KHR_portability_subset must be enabled if present in the implementation (e.g on macOS/iOS with beta extensions enabled)
    add_device_extension(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME, /*optional=*/true);
#endif

#ifdef DEBUG
    if (!debug_utils)
    {
        uint32_t extensionCount = 0;
        vkEnumerateDeviceExtensionProperties(gpu.get_handle(), nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> available_device_extensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(gpu.get_handle(), nullptr, &extensionCount,
                                             available_device_extensions.data());
        auto debugExtensionIt =
            std::find_if(available_device_extensions.begin(),
                         available_device_extensions.end(),
                         [](const VkExtensionProperties &ep)
                         {
                             return strcmp(ep.extensionName, VK_EXT_DEBUG_MARKER_EXTENSION_NAME) == 0;
                         });
        if (debugExtensionIt != available_device_extensions.end())
        {
            LOGI("Vulkan debug utils enabled ({})", VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
            AddDeviceExtension(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
        }
    }

    if (!debug_utils)
    {
        LOGW("Vulkan debug utils were requested, but no extension that provides them was found");
    }
#endif

    if (!debug_utils)
    {
        debug_utils = std::make_unique<vkb::DummyDebugUtils>();
    }
    device = CreateDevice(gpu);
    // VULKAN_HPP_DEFAULT_DISPATCHER.init(device->GetHandle());
    CreateRenderContext();
    render_context->prepare(1, vkb::RenderTarget::ONE_IMAGE_FUNC);

    // stats = std::make_unique<vkb::stats::HPPStats>(*render_context);

    // Start the sample in the first GUI configuration
    // configuration.reset();

    std::set<VkImageUsageFlagBits> usage = {VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT};
    GetRenderContext().update_swapchain(usage);

    vkb::GLTFLoader loader(*device);
    scene = loader.read_scene_from_file("Models/retroufo.gltf");

    auto &camera_node = vkb::add_free_camera(GetScene(), "main_camera", {512, 512});
    camera = dynamic_cast<vkb::sg::PerspectiveCamera *>(&camera_node.get_component<vkb::sg::Camera>());

    render_pipeline = CreateOneRenderpassTwoSubpasses();
    /*ViewportRTs.resize(GetRenderContext().get_render_frames().size());
    for (uint32_t i = 0; i < ViewportRTs.size(); i++)
    {
        ViewportRTs[i] = CreateRenderTarget({512, 512});
    }

    for (uint32_t i = 0; i < EditorUI->ViewportDescriptorSets.size(); i++)
    {
        EditorUI->ViewportDescriptorSets[i]
            = ImGui_ImplVulkan_AddTexture(EditorUI->OffScreenSampler->GetHandle(),
                                          ViewportRTs[i]->get_views()[0].GetHandle(),
                                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
    OffScreenResourcesReady = true;*/

    return true;
}

void RenderSystem::RequestGpuFeatures(vkb::PhysicalDevice &gpu)
{
    // To be overridden by sample
}

std::unique_ptr<vkb::Instance> RenderSystem::CreateInstance()
{
    return std::make_unique<vkb::Instance>("VulkanRenderer", GetInstanceExtensions(), GetInstanceLayers(),
                                           GetLayerSettings(), api_version);
}

std::unique_ptr<vkb::VulkanDevice> RenderSystem::CreateDevice(vkb::PhysicalDevice &gpu)
{
    return std::make_unique<vkb::VulkanDevice>(gpu, surface, std::move(debug_utils), GetDeviceExtensions());
}

void RenderSystem::Draw(vkb::CommandBuffer &command_buffer, vkb::RenderTarget &render_target)
{
    auto &views = render_target.get_views();

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

void RenderSystem::Render(vkb::CommandBuffer &command_buffer)
{
    if (render_pipeline)
    {
        render_pipeline->draw(command_buffer, render_context->get_active_frame().get_render_target());
    }
}

void RenderSystem::DrawRenderpass(vkb::CommandBuffer &command_buffer, vkb::RenderTarget &render_target)
{
    if (OffScreenResourcesReady)
    {
        auto &RT = *ViewportRTs[GetRenderContext().get_active_frame_index()];
        auto &views = RT.get_views();
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
    auto &framebuffer = device->get_resource_cache().request_framebuffer(render_target, *EditorUIRenderpass);
    renderPassBeginInfo.framebuffer = framebuffer.get_handle();
    vkCmdBeginRenderPass(command_buffer.GetHandle(), &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), command_buffer.GetHandle());
    vkCmdEndRenderPass(command_buffer.GetHandle());
}

void RenderSystem::Update(float delta_time)
{
    UpdateScene(delta_time);

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

void RenderSystem::UpdateScene(float delta_time)
{
    if (scene)
    {
        // Update scripts
        if (scene->has_component<vkb::sg::Script>())
        {
            auto scripts = scene->get_components<vkb::sg::Script>();

            for (auto script : scripts)
            {
                script->update(delta_time);
            }
        }

        // Update animations
        if (scene->has_component<vkb::sg::Animation>())
        {
            auto animations = scene->get_components<vkb::sg::Animation>();

            for (auto animation : animations)
            {
                animation->update(delta_time);
            }
        }
    }
}

void RenderSystem::UpdateDebugWindow()
{
    /*auto        driver_version     = device->get_gpu().get_driver_version();
    std::string driver_version_str = fmt::format("major: {} minor: {} patch: {}", driver_version.major, driver_version.minor, driver_version.patch);

    get_debug_info().template insert<field::Static, std::string>("driver_version", driver_version_str);
    get_debug_info().template insert<field::Static, std::string>("resolution",
                                                                 to_string(static_cast<VkExtent2D const &>(render_context->get_swapchain().get_extent())));
    get_debug_info().template insert<field::Static, std::string>("surface_format",
                                                                 to_string(render_context->get_swapchain().get_format()) + " (" +
                                                                     to_string(vkb::common::get_bits_per_pixel(render_context->get_swapchain().get_format())) +
                                                                     "bpp)");

    if (scene != nullptr)
    {
        get_debug_info().template insert<field::Static, uint32_t>("mesh_count", to_u32(scene->get_components<sg::SubMesh>().size()));
        get_debug_info().template insert<field::Static, uint32_t>("texture_count", to_u32(scene->get_components<sg::Texture>().size()));

        if (auto camera = scene->get_components<vkb::sg::Camera>()[0])
        {
            if (auto camera_node = camera->get_node())
            {
                const glm::vec3 &pos = camera_node->get_transform().get_translation();
                get_debug_info().template insert<field::Vector, float>("camera_pos", pos.x, pos.y, pos.z);
            }
        }
    }*/
}

void RenderSystem::Finish()
{
    if (device)
    {
        vkDeviceWaitIdle(device->GetHandle());
    }
}

void RenderSystem::SetViewportAndScissor(vkb::CommandBuffer const &command_buffer, VkExtent2D const &extent)
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

void RenderSystem::CreateRenderContext_Impl(const std::vector<VkSurfaceFormatKHR> &surface_priority_list)
{
    VkPresentModeKHR present_mode = (window->GetProperties().vsync == vkb::Window::Vsync::ON)
                                        ? VK_PRESENT_MODE_FIFO_KHR
                                        : VK_PRESENT_MODE_MAILBOX_KHR;
    std::vector<VkPresentModeKHR> present_mode_priority_list{
        VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR};
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
    /*if (!Parent::resize(width, height))
    {
        return false;
    }

    if (gui)
    {
        gui->resize(width, height);
    }

    if (scene && scene->has_component<sg::Script>())
    {
        auto scripts = scene->get_components<sg::Script>();

        for (auto script : scripts)
        {
            script->resize(width, height);
        }
    }

    if (stats)
    {
        stats->resize(width);
    }
    return true;*/
}

void RenderSystem::SetApiVersion(uint32_t requested_api_version)
{
    api_version = requested_api_version;
}

void RenderSystem::SetRenderContext(std::unique_ptr<vkb::RenderContext> &&rc)
{
    render_context.reset(rc.release());
}

void RenderSystem::SetRenderPipeline(std::unique_ptr<vkb::RenderPipeline> &&rp)
{
    render_pipeline.reset(rp.release());
}

void RenderSystem::AddDeviceExtension(const char *extension, bool optional)
{
    device_extensions[extension] = optional;
}

void RenderSystem::AddInstanceExtension(const char *extension, bool optional)
{
    instance_extensions[extension] = optional;
}

void RenderSystem::AddInstanceLayer(const char *layer, bool optional)
{
    instance_layers[layer] = optional;
}

void RenderSystem::AddLayerSetting(VkLayerSettingEXT const &layerSetting)
{
    layer_settings.push_back(layerSetting);
}

void RenderSystem::InitializeUIRenderBackend(EditorUIInterface *UIManager)
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

    this->UIManager->Prepare(EditorUIRenderpass->GetHandle(), device->get_suitable_graphics_queue().get_handle(), render_context->get_swapchain().get_images().size(), render_context->get_swapchain().get_images().size());
}

std::unique_ptr<vkb::RenderPipeline> RenderSystem::CreateOneRenderpassTwoSubpasses()
{
    // Geometry subpass
    auto geometry_vs = vkb::ShaderSource{Paths::GetShaderFullPath("deferred/geometry.vert.spv")};
    auto geometry_fs = vkb::ShaderSource{Paths::GetShaderFullPath("deferred/geometry.frag.spv")};
    auto scene_subpass = std::make_unique<vkb::GeometrySubpass>(GetRenderContext(), std::move(geometry_vs),
                                                                std::move(geometry_fs), GetScene(), *camera);

    // Outputs are depth, albedo, and normal
    scene_subpass->set_output_attachments({1, 2, 3});

    // Lighting subpass
    auto lighting_vs = vkb::ShaderSource{Paths::GetShaderFullPath("deferred/lighting.vert.spv")};
    auto lighting_fs = vkb::ShaderSource{Paths::GetShaderFullPath("deferred/lighting.frag.spv")};
    auto lighting_subpass = std::make_unique<vkb::LightingSubpass>(GetRenderContext(), std::move(lighting_vs),
                                                                   std::move(lighting_fs), *camera, GetScene(),
                                                                   ViewportRTs);

    // Inputs are depth, albedo, and normal from the geometry subpass
    lighting_subpass->set_input_attachments({1, 2, 3});

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
        VMA_MEMORY_USAGE_GPU_ONLY};

    vkb::Image depth_image{
        GetDevice(),
        extent,
        vkb::get_suitable_depth_format(GetDevice().get_gpu().get_handle()),
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | rt_usage_flags,
        VMA_MEMORY_USAGE_GPU_ONLY};

    vkb::Image albedo_image{
        GetDevice(),
        extent,
        albedo_format,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | rt_usage_flags,
        VMA_MEMORY_USAGE_GPU_ONLY};

    vkb::Image normal_image{
        GetDevice(),
        extent,
        normal_format,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | rt_usage_flags,
        VMA_MEMORY_USAGE_GPU_ONLY};

    std::vector<vkb::Image> images;

    // Attachment 0
    images.push_back(std::move(viewport_image));

    // Attachment 1
    images.push_back(std::move(depth_image));

    // Attachment 2
    images.push_back(std::move(albedo_image));

    // Attachment 3
    images.push_back(std::move(normal_image));

    return std::make_unique<vkb::RenderTarget>(std::move(images));
}

/*void RenderSystem::ViewportResize(ImVec2 size)
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

    for (uint32_t i = 0; i < EditorUI->ViewportDescriptorSets.size(); i++)
    {
        ImGui_ImplVulkan_RemoveTexture(EditorUI->ViewportDescriptorSets[i]);
        EditorUI->ViewportDescriptorSets[i]
            = ImGui_ImplVulkan_AddTexture(EditorUI->OffScreenSampler->GetHandle(),
                                          ViewportRTs[i]->get_views()[0].GetHandle(),
                                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
    camera->set_aspect_ratio(size.x / size.y);
    OffScreenResourcesReady = true;
}*/

void RenderSystem::DrawPipeline(vkb::CommandBuffer &command_buffer, vkb::RenderTarget &render_target,
                                vkb::RenderPipeline &render_pipeline)
{
    auto &extent = render_target.get_extent();

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

std::unordered_map<const char *, bool> const &RenderSystem::GetDeviceExtensions() const
{
    return device_extensions;
}

std::unordered_map<const char *, bool> const &RenderSystem::GetInstanceExtensions() const
{
    return instance_extensions;
}

std::unordered_map<const char *, bool> const &RenderSystem::GetInstanceLayers() const
{
    return instance_layers;
}

std::vector<VkLayerSettingEXT> const &RenderSystem::GetLayerSettings() const
{
    return layer_settings;
}

std::vector<VkSurfaceFormatKHR> const &RenderSystem::GetSurfacePriorityList() const
{
    return surface_priority_list;
}

std::vector<VkSurfaceFormatKHR> &RenderSystem::GetSurfacePriorityList()
{
    return surface_priority_list;
}
