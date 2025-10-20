#include "World/WorldManager.hpp"

#include "Engine/SceneGraph/Scene.hpp"
#include "Logging/Logger.hpp"
#include "Engine/SceneGraph/ComponentPool.hpp"

scene::Scene* WorldManager::CreateWorld(const std::string& name)
{
    auto world = std::make_unique<scene::Scene>(name);
    worlds[name] = std::move(world);
    return worlds[name].get();
}

bool WorldManager::LoadWorld(const std::string& name, const std::string& filePath)
{
    return false;
}

void WorldManager::SetActiveWorld(const std::string& name)
{
    activeWorld = GetWorld(name);
}

void WorldManager::DestroyWorld(const std::string& name)
{
    if (worlds.erase(name))
    {
        LOG_INFO("World destroyed: {} ", name)
    }
    else
    {
        LOG_WARN("World not found: {} ", name)
    }
}

scene::Scene* WorldManager::GetWorld(const std::string& name)
{
    auto it = worlds.find(name);
    return (it != worlds.end()) ? it->second.get() : nullptr;
}

void WorldManager::UpdateActiveWorld(float deltaTime)
{
    if (activeWorld)
    {
        //activeWorld->Update(deltaTime);
    }
}
