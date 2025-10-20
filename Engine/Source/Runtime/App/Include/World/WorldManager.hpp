#pragma once
#include <memory>
#include <string>
#include <unordered_map>

#include "Engine/SceneGraph/Scene.hpp"

class WorldManager
{
public:
    WorldManager() = default;
    ~WorldManager() = default;

    scene::Scene* CreateWorld(const std::string& name);
    bool LoadWorld(const std::string& name, const std::string& filePath);
    void SetActiveWorld(const std::string& name);
    void DestroyWorld(const std::string& name);

    scene::Scene* GetActiveWorld() { return activeWorld; }
    scene::Scene* GetWorld(const std::string& name);

    void UpdateActiveWorld(float deltaTime);

private:
    std::unordered_map<std::string, std::unique_ptr<scene::Scene>> worlds;
    scene::Scene* activeWorld = nullptr;
};
