#include "GlobalContext.hpp"
#include "WindowSystem.hpp"
#include "Engine/Asset/Manager/AssetManager.hpp"
#include "Rendering/RenderSystem.hpp"
#include "Engine/SceneGraph/Scene.hpp"
#include "Engine/SceneGraph/Node.hpp"
#include "World/WorldManager.hpp"
#include "Engine/SceneGraph/ComponentPool.hpp"
#include "Framework/Core/VulkanDevice.hpp"

RuntimeGlobalContext GRuntimeGlobalContext;

void RuntimeGlobalContext::StartSystems(const std::string& config_file_path)
{
}

void RuntimeGlobalContext::ShutdownSystems()
{
}
