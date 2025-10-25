#include "GlobalContext.hpp"
#include "WindowSystem.hpp"
#include "Render/RenderSystem.hpp"
#include "Engine/SceneGraph/Scene.hpp"
#include "Engine/SceneGraph/Node.hpp"
#include "World/WorldManager.hpp"
#include "Engine/SceneGraph/ComponentPool.hpp"

RuntimeGlobalContext GRuntimeGlobalContext;

void RuntimeGlobalContext::StartSystems(const std::string& config_file_path)
{
    vkb::Window::Properties window_properties;
    window_properties.title = "VkoraEngine";
    windowSystem = std::make_shared<WindowSystem>(window_properties);
    worldManager = std::make_shared<WorldManager>();
    renderSystem = std::make_shared<RenderSystem>();
}

void RuntimeGlobalContext::ShutdownSystems()
{
    renderSystem.reset();
    worldManager.reset();
    windowSystem.reset();
}
