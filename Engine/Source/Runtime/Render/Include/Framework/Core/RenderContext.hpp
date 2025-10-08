#pragma once
#include <string>
#include <vector>
#include <volk.h>
#include <Windows.h>

#include "benchmark.hpp"
#include "Camera.hpp"
#include "VulkanSwapChain.h"

struct VulkanDevice;

class RenderContext
{
private:
    uint32_t destWidth{};
    uint32_t destHeight{};
    std::string getWindowTitle() const;
    bool resizing = false;
    void handleMouseMove(int32_t x, int32_t y);
    void nextFrame();
    void updateOverlay();
    void createPipelineCache();
    void createCommandPool();
    void createSynchronizationPrimitives();
    void createSurface();
    void createSwapChain();
    void createCommandBuffers();
    void destroyCommandBuffers();
    std::string shaderDir = "glsl";

public:
    /** @brief State of mouse/touch input */
    struct
    {
        struct
        {
            bool left = false;
            bool right = false;
            bool middle = false;
        } buttons;
        glm::vec2 position;
    } mouseState;

    static std::vector<const char *> Args;
    /** @brief Example settings that can be changed e.g. by command line arguments */
    struct Settings
    {
        /** @brief Activates validation layers (and message output) when set to true */
        bool validation = false;
        /** @brief Set to true if fullscreen mode has been requested via command line */
        bool fullscreen = false;
        /** @brief Set to true if v-sync will be forced for the swapchain */
        bool vsync = false;
        /** @brief Enable UI overlay */
        bool overlay = true;
    } settings;
    Camera camera;
    std::string title = "Vulkan Render";
    std::string name = "vulkanRender";
    uint32_t apiVersion = VK_API_VERSION_1_0;
    uint32_t width = 1280;
    uint32_t height = 720;
    // UIOverlay ui;
    vks::Benchmark benchmark;
    // Defines a frame rate independent timer value clamped from -1.0...1.0
    // For use in animations, rotations, etc.
    float timer = 0.0f;
    // Multiplier for speeding up (or slowing down) the global timer
    float timerSpeed = 0.25f;
    bool paused = false;
    uint32_t lastFPS = 0;
    bool prepared = false;
    bool resized = false;
    bool viewUpdated = false;
    /** @brief Last frame time measured using a high performance timer (if available) */
    float frameTimer = 1.0f;
    /** @brief Encapsulated physical and logical vulkan device */
    VulkanDevice *vulkanDevice{};

    /** @brief Default depth stencil attachment used by the default render pass */
    struct
    {
        VkImage image;
        VkDeviceMemory memory;
        VkImageView view;
    } depthStencil{};

    virtual void OnHandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    /** @brief (Virtual) Called after a key was pressed, can be used to do custom key handling */
    virtual void keyPressed(uint32_t);
    void windowResize();
    bool InitVulkan();
    /** @brief (Virtual) Creates the application wide Vulkan instance */
    virtual VkResult CreateInstance();
    /** @brief (Virtual) Called after the physical device features have been read, can be used to set features to enable on the device */
    virtual void getEnabledFeatures();
    /** @brief (Virtual) Called after the physical device extensions have been read, can be used to enable extensions based on the supported extension listing*/
    virtual void getEnabledExtensions();
    /** @brief (Pure virtual) Render function to be implemented by the sample application */
    virtual void render() = 0;
    /** @brief Loads a SPIR-V shader file for the given shader stage */
    VkPipelineShaderStageCreateInfo loadShader(std::string fileName, VkShaderStageFlagBits stage);
    /** @brief Entry point for the main render loop */
    void renderLoop();
    /** @brief (Virtual) Called when resources have been recreated that require a rebuild of the command buffers (e.g. frame buffer), to be implemented by the sample application */
    virtual void buildCommandBuffers();
    /** @brief (Virtual) Called when the window has been resized, can be used by the sample application to recreate resources */
    virtual void windowResized();
    /** @brief (Virtual) Setup default framebuffers for all requested swapchain images */
    virtual void setupFrameBuffer();
    /** @brief (Virtual) Setup a default renderpass */
    virtual void setupRenderPass();
    /** @brief (Virtual) Setup default depth and stencil views */
    virtual void setupDepthStencil();
    /** @brief Prepares all Vulkan resources and functions required to run the sample */
    virtual void prepare();
    /** Prepare the next frame for workload submission by acquiring the next swap chain image */
    void prepareFrame();
    /** @brief Presents the current image to the swap chain */
    void submitFrame();
    /** @brief (Virtual) Called when the UI overlay is updating, can be used to add custom elements to the overlay */
    // virtual void OnUpdateUIOverlay(UIOverlay *overlay);
    /** @brief (Virtual) Called after the mouse cursor moved and before internal events (like camera rotation) is handled */
    virtual void mouseMoved(double x, double y, bool &handled);
    /** The Settings window has done what GLFW should do */
    HWND setupWindow(HINSTANCE hinstance, WNDPROC wndproc);
    void handleMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
    // Returns the path to the root of the glsl, hlsl or slang shader directory.
    std::string getShadersPath() const;
    HWND window;
    HINSTANCE windowInstance;
    std::chrono::time_point<std::chrono::high_resolution_clock> lastTimestamp, tPrevEnd;

    /** @brief Set of instance extensions to be enabled for this example (must be set in the derived constructor) */
    std::vector<const char *> enabledInstanceExtensions;
    std::vector<std::string> supportedInstanceExtensions;
    /** @brief Set of layer settings to be enabled for this example (must be set in the derived constructor) */
    std::vector<VkLayerSettingEXT> enabledLayerSettings;

    // Frame counter to display fps
    uint32_t frameCounter = 0;
    // Vulkan instance, stores all per-application states
    VkInstance instance{VK_NULL_HANDLE};
    // Physical device (GPU) that Vulkan will use
    VkPhysicalDevice physicalDevice{VK_NULL_HANDLE};
    // Stores physical device properties (for e.g. checking device limits)
    VkPhysicalDeviceProperties deviceProperties{};
    // Stores the features available on the selected physical device (for e.g. checking if a feature is available)
    VkPhysicalDeviceFeatures deviceFeatures{};
    // Stores all available memory (type) properties for the physical device
    VkPhysicalDeviceMemoryProperties deviceMemoryProperties{};
    /** @brief Set of physical device features to be enabled for this example (must be set in the derived constructor) */
    VkPhysicalDeviceFeatures enabledFeatures{};
    /** @brief Set of device extensions to be enabled for this example (must be set in the derived constructor) */
    std::vector<const char *> enabledDeviceExtensions;
    /** @brief Optional pNext structure for passing extension structures to device creation */
    void *deviceCreatepNextChain = nullptr;
    /** @brief Logical device, application's view of the physical device (GPU) */
    VkDevice device{VK_NULL_HANDLE};
    VkQueue queue{VK_NULL_HANDLE};
    // Depth buffer format (selected during Vulkan initialization)
    VkFormat depthFormat{VK_FORMAT_UNDEFINED};
    // Command buffer pool
    VkCommandPool cmdPool{VK_NULL_HANDLE};
    // Wraps the swap chain to present images (framebuffers) to the windowing system
    SwapChain swapChain;
    /** @brief Pipeline stages used to wait at for graphics queue submissions */
    VkPipelineStageFlags submitPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    // Contains command buffers and semaphores to be presented to the queue
    VkSubmitInfo submitInfo{};
    // Command buffers used for rendering
    std::vector<VkCommandBuffer> drawCmdBuffers;
    // Global render pass for frame buffer writes
    VkRenderPass renderPass{VK_NULL_HANDLE};
    struct
    {
        // Swap chain image presentation
        VkSemaphore presentComplete;
        // Command buffer submission and execution
        VkSemaphore renderComplete;
    } semaphores{};
    std::vector<VkFence> waitFences;
    // List of shader modules created (stored for cleanup)
    std::vector<VkShaderModule> shaderModules;
    // Pipeline cache object
    VkPipelineCache pipelineCache{VK_NULL_HANDLE};
    // List of available frame buffers (same as number of swap chain images)
    std::vector<VkFramebuffer> frameBuffers;

    // Descriptor set pool
    VkDescriptorPool descriptorPool{VK_NULL_HANDLE};

    // Active frame buffer index
    uint32_t currentBuffer = 0;

    RenderContext();
    virtual ~RenderContext();
    bool requiresStencil{false};
};

inline void RenderContext::mouseMoved(double x, double y, bool &handled)
{
}
