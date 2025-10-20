#include "GlobalContext.hpp"
#include "WindowSystem.hpp"
#include "Render/RenderSystem.hpp"
#include "SceneGraph/Scene.h"
#include "SceneGraph/Node.h"
#include "World/WorldManager.hpp"
#include "Engine/SceneGraph/ComponentPool.hpp"

RuntimeGlobalContext GRuntimeGlobalContext;

void RuntimeGlobalContext::StartSystems(const std::string& config_file_path)
{
    vkb::Window::Properties window_properties;
    window_properties.title = "VkoraEngine";
    windowSystem = std::make_shared<WindowSystem>(window_properties);
    renderSystem = std::make_shared<RenderSystem>();
    worldManager = std::make_shared<WorldManager>();
}

void RuntimeGlobalContext::ShutdownSystems()
{
    worldManager.reset();
    renderSystem.reset();
    windowSystem.reset();
}
