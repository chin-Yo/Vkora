#include "Engine/Engine.hpp"
#include "GlobalContext.hpp"
#include "Logging/Logger.hpp"
#include <iostream>
#include <atomic>
#include <chrono>
#include "Rendering/RenderSystem.hpp"
#include <algorithm>

#include "Engine/Asset/AssetImporter.hpp"
#include "Engine/Asset/AssetRegistry.hpp"
#include "Engine/Asset/Manager/AssetManager.hpp"
#include "Misc/Paths.hpp"
#include "World/WorldManager.hpp"
#include "Engine/SceneGraph/ComponentPool.hpp"
#include "UIManage/EditorGlobalContext.hpp"

const float Engine::FPSAlpha = 1.f / 100;

Engine *GEngine = nullptr;

void Engine::LogicalTick(float DeltaTime)
{
    worldManager->UpdateActiveWorld(DeltaTime);
}

bool Engine::RendererTick(float DeltaTime)
{
    renderSystem->Update(DeltaTime);
    return true;
}

void Engine::CalculateFPS(float DeltaTime)
{
    DeltaTime = std::min<float>(DeltaTime, 0.5f);
    FrameCount++;

    if (FrameCount == 1 || FrameCount % 1000 == 0)
    {
        AverageDuration = DeltaTime;
    }
    else
    {
        AverageDuration = AverageDuration * (1 - FPSAlpha) + DeltaTime * FPSAlpha;
    }
    FPS = static_cast<int>(1.f / AverageDuration);
}

float Engine::CalculateDeltaTime()
{
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - LastTickTimePoint);
    LastTickTimePoint = now;
    return duration.count() / 1000000.0f;
}

void Engine::LimitFPS(float& DeltaTime)
{
    if (MaxFPS <= 0)
        return; // No limit

    // Calculate the target frame time in seconds
    const float targetFrameTime = 1.0f / static_cast<float>(MaxFPS);

    // Calculate actual elapsed time
    auto frameEndTime = std::chrono::steady_clock::now();
    float actualFrameTime = std::chrono::duration_cast<std::chrono::microseconds>(frameEndTime - FrameStartTimePoint).
        count() / 1000000.0f;

    // If frame time is shorter than target, sleep for remaining time
    if (actualFrameTime < targetFrameTime)
    {
        float sleepTime = targetFrameTime - actualFrameTime;
        std::this_thread::sleep_for(std::chrono::microseconds(static_cast<long long>(sleepTime * 1000000)));

        // Update DeltaTime with the adjusted value
        DeltaTime = targetFrameTime;
    }

    // Update start time for next frame
    FrameStartTimePoint = std::chrono::steady_clock::now();
}

void Engine::SetIsIconify(bool bIsIconify)
{
    bIsMinimized = bIsIconify;
}

void Engine::VkBackend::AddDeviceExtension(const char* extension, bool optional)
{
    device_extensions[extension] = optional;
}

void Engine::VkBackend::AddInstanceExtension(const char* extension, bool optional)
{
    instance_extensions[extension] = optional;
}

void Engine::VkBackend::AddInstanceLayer(const char* layer, bool optional)
{
    instance_layers[layer] = optional;
}

void Engine::VkBackend::AddLayerSetting(VkLayerSettingEXT const& layerSetting)
{
    layer_settings.push_back(layerSetting);
}

void Engine::InitRenderBackend(const BackendOptions& options)
{
    LOG_INFO("Initializing vulkan render system!")
    assert(options.window != nullptr && "Window is invalid");

    bool headless = windowSystem->GetWindowMode() == vkb::Window::Mode::Headless;

    VK_CHECK_RESULT(volkInitialize());

    // Creating the vulkan instance
    for (const char* extension_name : windowSystem->GetRequiredSurfaceExtensions())
    {
        RenderBackend.AddInstanceExtension(extension_name);
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
                         [](VkExtensionProperties const& ep)
                         {
                             return strcmp(ep.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0;
                         });
        if (debugExtensionIt != available_instance_extensions.end())
        {
            LOGI("Vulkan debug utils enabled ({})", VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

            RenderBackend.debug_utils = std::make_unique<vkb::DebugUtilsExtDebugUtils>();
            RenderBackend.AddInstanceExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
    }
#endif

    RenderBackend.instance = std::make_unique<vkb::Instance>("VulkanRenderer", RenderBackend.instance_extensions,
                                                             RenderBackend.instance_layers,
                                                             RenderBackend.layer_settings, RenderBackend.api_version);
    // VULKAN_HPP_DEFAULT_DISPATCHER.init(instance->get_handle());
    RenderBackend.surface = windowSystem->CreateSurface(*RenderBackend.instance);
    if (!RenderBackend.surface)
    {
        LOG_ERROR("Failed to create window surface.");
    }

    auto& gpu = RenderBackend.instance->get_suitable_gpu(RenderBackend.surface, headless);
    gpu.set_high_priority_graphics_queue_enable(RenderBackend.high_priority_graphics_queue);

    if (gpu.get_features().textureCompressionASTC_LDR)
    {
        gpu.get_mutable_requested_features().textureCompressionASTC_LDR = true;
    }

    if (gpu.get_features().samplerAnisotropy)
    {
        gpu.get_mutable_requested_features().samplerAnisotropy = true;
    }

    //RequestGpuFeatures(gpu);

    // Creating vulkan device, specifying the swapchain extension always
    // If using VK_EXT_headless_surface, we still create and use a swap-chain
    {
        RenderBackend.AddDeviceExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        if (RenderBackend.instance_extensions.find(VK_KHR_DISPLAY_EXTENSION_NAME) != RenderBackend.instance_extensions.
            end())
        {
            RenderBackend.AddDeviceExtension(VK_KHR_DISPLAY_SWAPCHAIN_EXTENSION_NAME, /*optional=*/true);
        }
    }
    // TODO
#ifdef VK_ENABLE_PORTABILITY
    // VK_KHR_portability_subset must be enabled if present in the implementation (e.g on macOS/iOS with beta extensions enabled)
    add_device_extension(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME, /*optional=*/true);
#endif

#ifdef DEBUG
    if (!RenderBackend.debug_utils)
    {
        uint32_t extensionCount = 0;
        vkEnumerateDeviceExtensionProperties(gpu.get_handle(), nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> available_device_extensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(gpu.get_handle(), nullptr, &extensionCount,
                                             available_device_extensions.data());
        auto debugExtensionIt =
            std::find_if(available_device_extensions.begin(),
                         available_device_extensions.end(),
                         [](const VkExtensionProperties& ep)
                         {
                             return strcmp(ep.extensionName, VK_EXT_DEBUG_MARKER_EXTENSION_NAME) == 0;
                         });
        if (debugExtensionIt != available_device_extensions.end())
        {
            LOGI("Vulkan debug utils enabled ({})", VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
            RenderBackend.AddDeviceExtension(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
        }
    }

    if (!RenderBackend.debug_utils)
    {
        LOGW("Vulkan debug utils were requested, but no extension that provides them was found");
    }
#endif

    if (!RenderBackend.debug_utils)
    {
        RenderBackend.debug_utils = std::make_unique<vkb::DummyDebugUtils>();
    }
    RenderBackend.device = std::make_unique<vkb::VulkanDevice>(gpu, RenderBackend.surface,
                                                               std::move(RenderBackend.debug_utils),
                                                               RenderBackend.device_extensions);
}

void Engine::ShutdownRenderBackend()
{
    RenderBackend.device.reset();
    if (RenderBackend.surface)
    {
        vkDestroySurfaceKHR(RenderBackend.instance->get_handle(), RenderBackend.surface, nullptr);
    }
    RenderBackend.instance.reset();
}

void Engine::StartEngine(const std::string& ConfigFilePath)
{
    vkb::Window::Properties window_properties;
    window_properties.title = "VkoraEngine";
    windowSystem = std::make_unique<WindowSystem>(window_properties);

    InitRenderBackend({windowSystem.get()});

    worldManager = std::make_unique<WorldManager>();

    renderSystem = std::make_unique<
        RenderSystem>(windowSystem.get(), RenderBackend.device.get(), RenderBackend.surface);
    
    LOG_INFO("Engine started")
}

void Engine::ShutdownEngine()
{
    assetManager.reset();
    EditorManager->Shutdown();
    EditorManager.reset();
    renderSystem.reset();
    worldManager.reset();
    windowSystem.reset();
    ShutdownRenderBackend();
    LOG_INFO("Engine exit")
}

void Engine::Initialize()
{
    windowSystem->RegisterOnWindowIconifyFunc([this](bool bIsIconify)
        {
            if (this != nullptr)
                this->SetIsIconify(bIsIconify);
        }
    );
    if (!worldManager->GetActiveWorld())
    {
        worldManager->CreateWorld("DefaultWorld");
    }
    if (!renderSystem->Prepare())
    {
        LOG_CRITICAL("Prepare failed !!!")
        std::abort();
    }

    AssetImporter importer;
    importer.ScanAndImport(Paths::GetAssetPath());

    auto& assetRegistry = AssetRegistry::Get();
    assetRegistry.ScanDirectory(Paths::GetContentPath());

    assetManager = std::make_unique<AssetManager>(RenderBackend.GetDevice());
    
    GRuntimeGlobalContext.device = RenderBackend.device.get();
    GRuntimeGlobalContext.renderSystem = renderSystem.get();
    GRuntimeGlobalContext.worldManager = worldManager.get();
    GRuntimeGlobalContext.windowSystem = windowSystem.get();
    GRuntimeGlobalContext.assetManager = assetManager.get();


    EditorManager = std::make_unique<EditorUIManager>(renderSystem->GetDevice());
    renderSystem->InitializeUIRenderBackend(EditorManager.get());
    EditorManager->Initialize();
    GEditorGlobalContext.Initialize({EditorManager.get()});
}

void Engine::Clear()
{
}

void Engine::Tick()
{
    assetManager->LodaAllTexture();
    renderSystem->RenderPrepare();

    float delta_time;
    while (true)
    {
        delta_time = CalculateDeltaTime();
        LimitFPS(delta_time);

        if (!TickOneFrame(delta_time))
            return;
    }
}

bool Engine::TickOneFrame(float DeltaTime)
{
    LogicalTick(DeltaTime);
    CalculateFPS(DeltaTime);

    if (!bIsMinimized)
    {
        RendererTick(DeltaTime);
    }

    windowSystem->ProcessEvents();
    windowSystem->SetTitle(
        std::string("VkoraEngine - " + std::to_string(GetFPS()) + " FPS").c_str());
    const bool should_window_close = windowSystem->ShouldClose();
    if (should_window_close)
    {
        renderSystem->Finish();
    }
    return !should_window_close;
}

std::string Engine::GetEngineStatus() const
{
    return std::string();
}
