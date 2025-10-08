/*
#include "Render/EditorUI.hpp"

#include <cmath>
#include <stb_image.h>

#include "Framework/Core/VulkanTools.hpp"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#include "GlobalContext.hpp"
#include "Constants/Epsilon.hpp"
#include "Framework/Core/RenderPass.hpp"
#include "Render/RenderSystem.hpp"
#include "GlobalContext.hpp"
#include "Engine/Preset/VkPreset.hpp"
#include "Framework/Core/CommandBuffer.hpp"
#include "Framework/Core/Queue.hpp"
#include "Misc/Paths.hpp"
#include "SceneGraph/Components/Image.h"

EditorUIManager::EditorUIManager(vkb::VulkanDevice& device)
    : device(device), descriptorPool(vks::DescriptorPoolBuilder(device.GetHandle())
                                     .setMaxSets(10)
                                     .addPoolSize(VK_DESCRIPTOR_TYPE_SAMPLER, 1000)
                                     .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000)
                                     .addPoolSize(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000)
                                     .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000)
                                     .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000)
                                     .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000)
                                     .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000)
                                     .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000)
                                     .addPoolSize(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000)
                                     .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000)
                                     .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000)
                                     .setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
                                     .build())

{
    OffScreenSampler = new vkb::Sampler({device, vks::initializers::samplerCreateInfo()});
    ViewportDescriptorSets.resize(3);
}

EditorUIManager::~EditorUIManager()
{
    delete OffScreenSampler;
    for (uint32_t i = 0; i < ViewportDescriptorSets.size(); i++)
    {
        ImGui_ImplVulkan_RemoveTexture(ViewportDescriptorSets[i]);
    }
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void EditorUIManager::Prepare(VkRenderPass renderPass, VkQueue queue, uint32_t MinImageCount,
                              uint32_t ImageCount)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;

    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable Docking
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();
    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
    ImGui_ImplGlfw_InitForVulkan(GRuntimeGlobalContext.windowSystem->GetWindow(), true);

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = device.get_gpu().get_instance().get_handle();
    init_info.PhysicalDevice = device.get_gpu().get_handle();
    init_info.Device = device.GetHandle();
    init_info.QueueFamily = device.get_queue_family_index(VK_QUEUE_GRAPHICS_BIT);
    init_info.Queue = queue;
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = descriptorPool.get();
    init_info.Subpass = 0;
    init_info.MinImageCount = MinImageCount;
    init_info.ImageCount = ImageCount;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = nullptr;
    init_info.CheckVkResultFn = nullptr;
    init_info.RenderPass = renderPass;

    render_pass = renderPass;

    bool bIsInit = ImGui_ImplVulkan_Init(&init_info);
    if (!bIsInit)
    {
        LOG_CRITICAL("Imgui initialization failed");
        assert(false && "Imgui initialization failed");
        std::abort();
    }

    /*auto tmpTx = ps::load_texture(device,Paths::GetAssetFullPath("Textures/demo1_22.png"),vkb::sg::Image::Unknown);
    tmpImage = std::move(tmpTx.image);

    tmpDs = ImGui_ImplVulkan_AddTexture(OffScreenSampler->GetHandle(), tmpImage->get_vk_image_view().GetHandle(),
                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);#1#
}

void EditorUIManager::Draw(VkCommandBuffer command_buffer)
{
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), command_buffer);


}

/** Update vertex and index buffer containing the imGui elements when required #1#
bool EditorUIManager::update()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    RenderMenuBar();
    RenderHierarchy();
    RenderInspector();
    RenderProjectBrowser();
    RenderConsole();
    RenderViewport(true);

    ImGui::Render();
    return true;
}

void EditorUIManager::resize(uint32_t width, uint32_t height)
{
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)(width), (float)(height));
}

void EditorUIManager::freeResources()
{
}

void EditorUIManager::RenderMenuBar()
{
    static bool dockspaceOpen = true;
    static bool opt_fullscreen = true;
    static bool opt_padding = false;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
    // because it would be confusing to have two docking targets within each others.
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    if (opt_fullscreen)
    {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    }
    else
    {
        dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
    }

    // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
    // and handle the pass-thru hole, so we ask Begin() to not render a background.
    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

    // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
    // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
    // all active windows docked into it will lose their parent and become undocked.
    // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
    // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
    if (!opt_padding)
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("DockSpace ImGui", &dockspaceOpen, window_flags);

    if (!opt_padding)
        ImGui::PopStyleVar();

    if (opt_fullscreen)
        ImGui::PopStyleVar(2);

    // Submit the DockSpace
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New Scene"))
            {
            }
            if (ImGui::MenuItem("Open Scene"))
            {
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit"))
            {
                // glfwSetWindowShouldClose(window, true);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem("Undo", "CTRL+Z"))
            {
            }
            if (ImGui::MenuItem("Redo", "CTRL+Y"))
            {
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Window"))
        {
            if (ImGui::BeginMenu("UI Style"))
            {
                static int selectedStyle = 0;

                if (ImGui::MenuItem("Dark", nullptr, selectedStyle == 0))
                {
                    selectedStyle = 0;
                    ImGui::StyleColorsDark();
                }

                if (ImGui::MenuItem("Light", nullptr, selectedStyle == 1))
                {
                    selectedStyle = 1;
                    ImGui::StyleColorsLight();
                }

                if (ImGui::MenuItem("Classic", nullptr, selectedStyle == 2))
                {
                    selectedStyle = 2;
                    ImGui::StyleColorsClassic();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Options"))
        {
            // Disabling fullscreen would allow the window to be moved to the front of other windows,
            // which we can't undo at the moment without finer window depth/z control.
            ImGui::MenuItem("Fullscreen", NULL, &opt_fullscreen);
            ImGui::MenuItem("Padding", NULL, &opt_padding);
            ImGui::Separator();

            if (ImGui::MenuItem("Flag: NoDockingOverCentralNode", "",
                                (dockspace_flags & ImGuiDockNodeFlags_NoDockingOverCentralNode) != 0))
            {
                dockspace_flags ^= ImGuiDockNodeFlags_NoDockingOverCentralNode;
            }
            if (ImGui::MenuItem("Flag: NoDockingSplit", "", (dockspace_flags & ImGuiDockNodeFlags_NoDockingSplit) != 0))
            {
                dockspace_flags ^= ImGuiDockNodeFlags_NoDockingSplit;
            }
            if (ImGui::MenuItem("Flag: NoUndocking", "", (dockspace_flags & ImGuiDockNodeFlags_NoUndocking) != 0))
            {
                dockspace_flags ^= ImGuiDockNodeFlags_NoUndocking;
            }
            if (ImGui::MenuItem("Flag: NoResize", "", (dockspace_flags & ImGuiDockNodeFlags_NoResize) != 0))
            {
                dockspace_flags ^= ImGuiDockNodeFlags_NoResize;
            }
            if (ImGui::MenuItem("Flag: AutoHideTabBar", "", (dockspace_flags & ImGuiDockNodeFlags_AutoHideTabBar) != 0))
            {
                dockspace_flags ^= ImGuiDockNodeFlags_AutoHideTabBar;
            }
            if (ImGui::MenuItem("Flag: PassthruCentralNode", "",
                                (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) != 0, opt_fullscreen))
            {
                dockspace_flags ^= ImGuiDockNodeFlags_PassthruCentralNode;
            }
            ImGui::Separator();

            if (ImGui::MenuItem("Close", NULL, false, &dockspaceOpen != NULL))
                dockspaceOpen = false;
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
    ImGui::End();
}

namespace EditorState
{
    int selected_node = -1;
    std::vector<std::string> console_logs;
}

void EditorUIManager::RenderHierarchy()
{
    ImGui::Begin("Hierarchy");

    const char* nodes[] = {"Camera", "Directional Light", "Player", "Enemy_1", "Enemy_2", "Ground", "Environment"};
    for (int i = 0; i < IM_ARRAYSIZE(nodes); ++i)
    {
        if (ImGui::Selectable(nodes[i], EditorState::selected_node == i))
        {
            EditorState::selected_node = i;
            EditorState::console_logs.push_back("[INFO] Selected '" + std::string(nodes[i]) + "' in Hierarchy.");
        }
    }

    ImGui::End();
}

void EditorUIManager::RenderInspector()
{
    ImGui::Begin("Inspector");

    if (EditorState::selected_node != -1)
    {
        ImGui::Text("Properties of object %d", EditorState::selected_node);
        ImGui::Separator();

        static float pos[3] = {0.0f, 0.0f, 0.0f};
        static float rot[3] = {0.0f, 0.0f, 0.0f};
        static float scale[3] = {1.0f, 1.0f, 1.0f};

        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::DragFloat3("Position", pos, 0.1f);
            ImGui::DragFloat3("Rotation", rot, 1.0f);
            ImGui::DragFloat3("Scale", scale, 0.1f);
        }

        if (ImGui::CollapsingHeader("Mesh Renderer"))
        {
            ImGui::Text("Mesh: Cube.obj");
            ImGui::Text("Material: DefaultMaterial");
        }

        if (ImGui::Button("Add Component"))
        {
        }
    }
    else
    {
        ImGui::Text("Select an object in the Hierarchy to see its properties.");
    }

    ImGui::End();
}

void EditorUIManager::RenderProjectBrowser()
{
    ImGui::Begin("Project Browser");

    ImGui::Text("[DIR] Assets");
    ImGui::Indent();
    ImGui::Text("[DIR] Models");
    ImGui::Text("[DIR] Textures");
    ImGui::Text("[TEX] Player_Albedo.png");
    ImGui::Text("[MAT] Ground_Material.mat");
    ImGui::Text("[SCN] MainScene.scene");
    ImGui::Unindent();

    ImGui::End();
}

void EditorUIManager::RenderConsole()
{
    ImGui::Begin("Console");

    if (ImGui::Button("Clear"))
    {
        EditorState::console_logs.clear();
    }
    ImGui::Separator();

    ImGui::BeginChild("LogScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
    for (const auto& log : EditorState::console_logs)
    {
        ImGui::TextUnformatted(log.c_str());
    }
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
    {
        ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();

    ImGui::End();
}

void EditorUIManager::RenderViewport(bool off)
{
    ImGui::SetNextWindowSize(ImVec2(500, 600), ImGuiCond_Once);
    ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::Text("Viewport fuck vulkan");
    ImVec2 currentViewportSize = ImGui::GetContentRegionAvail();

    bool hasChanged = (std::abs(ViewportSize.x - currentViewportSize.x) > 2) ||
        (std::abs(ViewportSize.y - currentViewportSize.y) > 2);
    if (currentViewportSize.x > 0.0f && currentViewportSize.y > 0.0f && hasChanged)
    {
        ViewportSize = currentViewportSize;
        ViewportResized = true;

        LOG_INFO("Viewport resized to {} x {}", ViewportSize.x, ViewportSize.y)
        OnViewportChange(currentViewportSize);
    }
    auto index = GRuntimeGlobalContext.renderSystem->GetRenderContext().get_active_frame_index();
    if (ViewportDescriptorSets[index] != nullptr)
    {
        ImGui::Image(ViewportDescriptorSets[index], ViewportSize);
    }

    ImGui::End();
}
*/
