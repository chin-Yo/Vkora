#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <array>
#include <functional>
#include <vector>
#include <tuple>

#include "Framework/Platform/Window.hpp"

struct WindowCreateInfo
{
    int width{1280};
    int height{720};
    const char* title{"Piccolo"};
    bool is_fullscreen{false};
};

class WindowSystem : public vkb::Window
{
public:
    WindowSystem(const Window::Properties& properties);
    ~WindowSystem() override;

    VkSurfaceKHR CreateSurface(vkb::Instance& instance) override;
    VkSurfaceKHR CreateSurface(VkInstance instance, VkPhysicalDevice physical_device) override;

    std::vector<const char*> GetRequiredSurfaceExtensions() const override;

    bool ShouldClose() override;

    void ProcessEvents() override;

    void Close() override;

    float GetDpiFactor() const override;

    float GetContentScaleFactor() const override;

    void SetTitle(const char* title);
    GLFWwindow* GetWindow() const;
    std::tuple<uint32_t, uint32_t> GetWindowSize() const;

    typedef std::function<void()> OnResetFunc;
    typedef std::function<void(int, int, int, int)> OnKeyFunc;
    typedef std::function<void(unsigned int)> OnCharFunc;
    typedef std::function<void(int, unsigned int)> OnCharModsFunc;
    typedef std::function<void(int, int, int)> OnMouseButtonFunc;
    typedef std::function<void(double, double)> OnCursorPosFunc;
    typedef std::function<void(int)> OnCursorEnterFunc;
    typedef std::function<void(double, double)> OnScrollFunc;
    typedef std::function<void(int, const char**)> OnDropFunc;
    typedef std::function<void(int, int)> OnWindowSizeFunc;
    typedef std::function<void()> OnWindowCloseFunc;
    typedef std::function<void(bool)> OnWindowIconifyFunc;

    void RegisterOnResetFunc(OnResetFunc func) { ResetFuncs.push_back(func); }
    void RegisterOnKeyFunc(OnKeyFunc func) { KeyFuncs.push_back(func); }
    void RegisterOnCharFunc(OnCharFunc func) { CharFuncs.push_back(func); }
    void RegisterOnCharModsFunc(OnCharModsFunc func) { CharModsFuncs.push_back(func); }
    void RegisterOnMouseButtonFunc(OnMouseButtonFunc func) { MouseButtonFuncs.push_back(func); }
    void RegisterOnCursorPosFunc(OnCursorPosFunc func) { CursorPosFuncs.push_back(func); }
    void RegisterOnCursorEnterFunc(OnCursorEnterFunc func) { CursorEnterFuncs.push_back(func); }
    void RegisterOnScrollFunc(OnScrollFunc func) { ScrollFuncs.push_back(func); }
    void RegisterOnDropFunc(OnDropFunc func) { DropFuncs.push_back(func); }
    void RegisterOnWindowSizeFunc(OnWindowSizeFunc func) { WindowSizeFuncs.push_back(func); }
    void RegisterOnWindowCloseFunc(OnWindowCloseFunc func) { WindowCloseFuncs.push_back(func); }
    void RegisterOnWindowIconifyFunc(OnWindowIconifyFunc func) { WindowIconifyFuncs.push_back(func); }

    bool IsMouseButtonDown(int button) const
    {
        if (button < GLFW_MOUSE_BUTTON_1 || button > GLFW_MOUSE_BUTTON_LAST)
        {
            return false;
        }
        return glfwGetMouseButton(glfwWindow, button) == GLFW_PRESS;
    }

    bool GetFocusMode() const { return bIsFocusMode; }
    void SetFocusMode(bool mode);

protected:
#pragma region callbacks
    // window event callbacks
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        WindowSystem* app = (WindowSystem*)glfwGetWindowUserPointer(window);
        if (app)
        {
            app->OnKey(key, scancode, action, mods);
        }
    }

    static void CharCallback(GLFWwindow* window, unsigned int codepoint)
    {
        WindowSystem* app = (WindowSystem*)glfwGetWindowUserPointer(window);
        if (app)
        {
            app->OnChar(codepoint);
        }
    }

    static void CharModsCallback(GLFWwindow* window, unsigned int codepoint, int mods)
    {
        WindowSystem* app = (WindowSystem*)glfwGetWindowUserPointer(window);
        if (app)
        {
            app->OnCharMods(codepoint, mods);
        }
    }

    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
    {
        WindowSystem* app = (WindowSystem*)glfwGetWindowUserPointer(window);
        if (app)
        {
            app->OnMouseButton(button, action, mods);
        }
    }

    static void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
    {
        WindowSystem* app = (WindowSystem*)glfwGetWindowUserPointer(window);
        if (app)
        {
            app->OnCursorPos(xpos, ypos);
        }
    }

    static void CursorEnterCallback(GLFWwindow* window, int entered)
    {
        WindowSystem* app = (WindowSystem*)glfwGetWindowUserPointer(window);
        if (app)
        {
            app->OnCursorEnter(entered);
        }
    }

    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
    {
        WindowSystem* app = (WindowSystem*)glfwGetWindowUserPointer(window);
        if (app)
        {
            app->OnScroll(xoffset, yoffset);
        }
    }

    static void DropCallback(GLFWwindow* window, int count, const char** paths)
    {
        WindowSystem* app = (WindowSystem*)glfwGetWindowUserPointer(window);
        if (app)
        {
            app->OnDrop(count, paths);
        }
    }

    static void WindowSizeCallback(GLFWwindow* window, int width, int height)
    {
        WindowSystem* app = (WindowSystem*)glfwGetWindowUserPointer(window);
        if (app)
        {
            app->width = width;
            app->height = height;
        }
    }

    static void WindowCloseCallback(GLFWwindow* window) { glfwSetWindowShouldClose(window, true); }

    static void WindowIconifyCallback(GLFWwindow* window, int iconified)
    {
        WindowSystem* app = (WindowSystem*)glfwGetWindowUserPointer(window);
        if (app)
        {
            app->OnWindowIconify(iconified);
        }
    }

    void OnReset()
    {
        for (auto& func : ResetFuncs)
            func();
    }

    void OnKey(int key, int scancode, int action, int mods)
    {
        for (auto& func : KeyFuncs)
            func(key, scancode, action, mods);
    }

    void OnChar(unsigned int codepoint)
    {
        for (auto& func : CharFuncs)
            func(codepoint);
    }

    void OnCharMods(int codepoint, unsigned int mods)
    {
        for (auto& func : CharModsFuncs)
            func(codepoint, mods);
    }

    void OnMouseButton(int button, int action, int mods)
    {
        for (auto& func : MouseButtonFuncs)
            func(button, action, mods);
    }

    void OnCursorPos(double xpos, double ypos)
    {
        for (auto& func : CursorPosFuncs)
            func(xpos, ypos);
    }

    void OnCursorEnter(int entered)
    {
        for (auto& func : CursorEnterFuncs)
            func(entered);
    }

    void OnScroll(double xoffset, double yoffset)
    {
        for (auto& func : ScrollFuncs)
            func(xoffset, yoffset);
    }

    void OnDrop(int count, const char** paths)
    {
        for (auto& func : DropFuncs)
            func(count, paths);
    }

    void OnWindowSize(int width, int height)
    {
        for (auto& func : WindowSizeFuncs)
            func(width, height);
    }

    void OnWindowIconify(int iconified)
    {
        for (auto& func : WindowIconifyFuncs)
        {
            func(iconified == GLFW_TRUE);
        }
    }
#pragma endregion

private:
    GLFWwindow* glfwWindow{nullptr};
    int width{0};
    int height{0};

    bool bIsFocusMode{false};

    std::vector<OnResetFunc> ResetFuncs;
    std::vector<OnKeyFunc> KeyFuncs;
    std::vector<OnCharFunc> CharFuncs;
    std::vector<OnCharModsFunc> CharModsFuncs;
    std::vector<OnMouseButtonFunc> MouseButtonFuncs;
    std::vector<OnCursorPosFunc> CursorPosFuncs;
    std::vector<OnCursorEnterFunc> CursorEnterFuncs;
    std::vector<OnScrollFunc> ScrollFuncs;
    std::vector<OnDropFunc> DropFuncs;
    std::vector<OnWindowSizeFunc> WindowSizeFuncs;
    std::vector<OnWindowCloseFunc> WindowCloseFuncs;
    std::vector<OnWindowIconifyFunc> WindowIconifyFuncs;
};
