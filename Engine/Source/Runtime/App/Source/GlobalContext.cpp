#include "GlobalContext.hpp"
#include "WindowSystem.hpp"
#include "Render/RenderSystem.hpp"
#include "SceneGraph/Scene.h"
#include "SceneGraph/Node.h"

RuntimeGlobalContext GRuntimeGlobalContext;

void RuntimeGlobalContext::StartSystems(const std::string& config_file_path)
{
    vkb::Window::Properties window_properties;
    window_properties.title = "VkoraEngine";
    windowSystem = std::make_shared<WindowSystem>(window_properties);
    renderSystem = std::make_shared<RenderSystem>();
}

void RuntimeGlobalContext::ShutdownSystems()
{
    renderSystem.reset();
    windowSystem.reset();
}
