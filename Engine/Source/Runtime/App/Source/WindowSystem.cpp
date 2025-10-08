#include "WindowSystem.hpp"

#include "Logging/Logger.hpp"

void error_callback(int error, const char* description)
{
    LOGE("GLFW Error (code {}): {}", error, description);
}

WindowSystem::WindowSystem(const Window::Properties& properties)
    : vkb::Window(properties)
{
    if (!glfwInit())
    {
        LOG_CRITICAL("GLFW couldn't be initialized.")
        std::abort();
    }

    glfwSetErrorCallback(error_callback);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    switch (properties.mode)
    {
    case Window::Mode::Fullscreen:
        {
            auto* monitor = glfwGetPrimaryMonitor();
            const auto* mode = glfwGetVideoMode(monitor);
            glfwWindow = glfwCreateWindow(mode->width, mode->height, properties.title.c_str(), monitor, NULL);
            break;
        }

    case Window::Mode::FullscreenBorderless:
        {
            auto* monitor = glfwGetPrimaryMonitor();
            const auto* mode = glfwGetVideoMode(monitor);
            glfwWindowHint(GLFW_RED_BITS, mode->redBits);
            glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
            glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
            glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
            glfwWindow = glfwCreateWindow(mode->width, mode->height, properties.title.c_str(), monitor, NULL);
            break;
        }

    case Window::Mode::FullscreenStretch:
        {
            throw std::runtime_error("Cannot support stretch mode on this platform.");
            break;
        }

    default:
        glfwWindow = glfwCreateWindow(properties.extent.width, properties.extent.height, properties.title.c_str(), NULL,
                                      NULL);
        break;
    }

    Resize(Extent{properties.extent.width, properties.extent.height});

    if (!glfwWindow)
    {
        LOG_CRITICAL("Couldn't create glfw window.")
        std::abort();
    }

    glfwSetWindowUserPointer(glfwWindow, this);

    glfwSetKeyCallback(glfwWindow, KeyCallback);
    glfwSetCharCallback(glfwWindow, CharCallback);
    glfwSetCharModsCallback(glfwWindow, CharModsCallback);
    glfwSetMouseButtonCallback(glfwWindow, MouseButtonCallback);
    glfwSetCursorPosCallback(glfwWindow, CursorPosCallback);
    glfwSetCursorEnterCallback(glfwWindow, CursorEnterCallback);
    glfwSetScrollCallback(glfwWindow, ScrollCallback);
    glfwSetDropCallback(glfwWindow, DropCallback);
    glfwSetWindowSizeCallback(glfwWindow, WindowSizeCallback);
    glfwSetWindowCloseCallback(glfwWindow, WindowCloseCallback);
    glfwSetWindowIconifyCallback(glfwWindow, WindowIconifyCallback);

    glfwSetInputMode(glfwWindow, GLFW_STICKY_KEYS, 1);
    glfwSetInputMode(glfwWindow, GLFW_STICKY_MOUSE_BUTTONS, 1);
}

WindowSystem::~WindowSystem()
{
    glfwDestroyWindow(glfwWindow);
    glfwTerminate();
}

VkSurfaceKHR WindowSystem::CreateSurface(vkb::Instance& instance)
{
    return CreateSurface(instance.get_handle(), VK_NULL_HANDLE);
}

VkSurfaceKHR WindowSystem::CreateSurface(VkInstance instance, VkPhysicalDevice physical_device)
{
    if (instance == VK_NULL_HANDLE || !glfwWindow)
    {
        return VK_NULL_HANDLE;
    }

    VkSurfaceKHR surface;

    VkResult errCode = glfwCreateWindowSurface(instance, glfwWindow, NULL, &surface);

    if (errCode != VK_SUCCESS)
    {
        return nullptr;
    }

    return surface;
}

std::vector<const char*> WindowSystem::GetRequiredSurfaceExtensions() const
{
    uint32_t glfw_extension_count{0};
    const char** names = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    return {names, names + glfw_extension_count};
}

void WindowSystem::ProcessEvents() { glfwPollEvents(); }

void WindowSystem::Close()
{
    glfwSetWindowShouldClose(glfwWindow, GLFW_TRUE);
}

float WindowSystem::GetDpiFactor() const
{
    auto primary_monitor = glfwGetPrimaryMonitor();
    auto vidmode = glfwGetVideoMode(primary_monitor);

    int width_mm, height_mm;
    glfwGetMonitorPhysicalSize(primary_monitor, &width_mm, &height_mm);

    // As suggested by the GLFW monitor guide
    static const float inch_to_mm = 25.0f;
    static const float win_base_density = 96.0f;

    auto dpi = static_cast<uint32_t>(vidmode->width / (width_mm / inch_to_mm));
    auto dpi_factor = dpi / win_base_density;
    return dpi_factor;
}

float WindowSystem::GetContentScaleFactor() const
{
    int fb_width, fb_height;
    glfwGetFramebufferSize(glfwWindow, &fb_width, &fb_height);
    int win_width, win_height;
    glfwGetWindowSize(glfwWindow, &win_width, &win_height);

    // We could return a 2D result here instead of a scalar,
    // but non-uniform scaling is very unlikely, and would
    // require significantly more changes in the IMGUI integration
    return static_cast<float>(fb_width) / win_width;
}

bool WindowSystem::ShouldClose() { return glfwWindowShouldClose(glfwWindow); }

void WindowSystem::SetTitle(const char* title) { glfwSetWindowTitle(glfwWindow, title); }

GLFWwindow* WindowSystem::GetWindow() const { return glfwWindow; }

std::tuple<uint32_t, uint32_t> WindowSystem::GetWindowSize() const { return {width, height}; }

void WindowSystem::SetFocusMode(bool mode)
{
    bIsFocusMode = mode;
    glfwSetInputMode(glfwWindow, GLFW_CURSOR, bIsFocusMode ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
}
