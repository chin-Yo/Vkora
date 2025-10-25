#include "UIManage/EditorUIManager.hpp"

#include "GlobalContext.hpp"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#include "Framework/Core/DescriptorPool.hpp"
#include "Framework/Core/Sampler.hpp"
#include "Framework/Core/VulkanDevice.hpp"
#include "Panel/DetailsPanel.hpp"
#include "Panel/FileBrowser.hpp"
#include "Panel/HierarchyPanel.hpp"
#include "Panel/MenuBar.hpp"
#include "Panel/Viewport.hpp"
#include "Render/RenderSystem.hpp"

EditorUIManager::EditorUIManager(vkb::VulkanDevice& device)
    : device(device)

{
    descriptorPool = vks::DescriptorPoolBuilder(device.GetHandle())
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
                     .buildRaw();
}

EditorUIManager::~EditorUIManager()
{
}

void EditorUIManager::Initialize()
{
    EditorPanels.push_back(std::make_shared<MenuBar>());
    EditorPanels.push_back(std::make_shared<HierarchyPanel>());
    EditorPanels.push_back(std::make_shared<FileBrowser>());
    EditorPanels.push_back(std::make_shared<ViewportPanel>());
    EditorPanels.push_back(std::make_shared<DetailsPanel>());
    GRuntimeGlobalContext.renderSystem->InitializeUIRenderBackend(this);
}

void EditorUIManager::Prepare(VkRenderPass renderPass, VkQueue queue, uint32_t MinImageCount, uint32_t ImageCount)
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
    init_info.DescriptorPool = descriptorPool;
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
}

void EditorUIManager::Shutdown()
{
    EditorPanels.clear();
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    vkDestroyDescriptorPool(device.GetHandle(), descriptorPool, nullptr);
}

void EditorUIManager::BeginFrame()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void EditorUIManager::EndFrame()
{
    ImGui::Render();
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

void EditorUIManager::RenderUI()
{
    for (const auto& panel : EditorPanels)
    {
        panel->OnUIRender();
    }
}
