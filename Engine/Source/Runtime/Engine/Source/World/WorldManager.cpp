#include "World/WorldManager.hpp"

#include "GlobalContext.hpp"
#include "Engine/Asset/Manager/AssetManager.hpp"
#include "Engine/SceneGraph/Scene.hpp"
#include "Logging/Logger.hpp"
#include "Engine/SceneGraph/ComponentPool.hpp"
#include "Engine/SceneGraph/Components/Material.hpp"
#include "Engine/SceneGraph/Components/PerspectiveCamera.hpp"
#include "Engine/SceneGraph/Components/Skybox.hpp"
#include "Engine/SceneGraph/Components/SubMesh.hpp"

scene::Scene* WorldManager::CreateWorld(const std::string& name)
{
    auto world = std::make_unique<scene::Scene>(name);

    auto cameraNode = std::make_unique<scene::Node>(world.get(), "DefaultCamera");
    ViewportCamera = world->GetComponentManager()->AddComponent<scene::PerspectiveCamera>(cameraNode.get());
    ViewportCamera->SetFarPlane(10000.0f);
    world->AddNode(std::move(cameraNode));

    auto SkyboxNode = std::make_unique<scene::Node>(world.get(), "Skybox");
    auto* SkyboxPtr = world->GetComponentManager()->AddComponent<scene::Skybox>(SkyboxNode.get());
    world->AddNode(std::move(SkyboxNode));

    auto CerNode = std::make_unique<scene::Node>(world.get(), "Cer");
    auto* CerPtr = world->GetComponentManager()->AddComponent<scene::SubMesh>(CerNode.get());
    CerPtr->meshData = GRuntimeGlobalContext.assetManager->GetMesh("Models/cerberus/cerberus.gltf");
    CerPtr->bHasMeshData = true;
    CerPtr->get_mut_material()->normal_texture = GRuntimeGlobalContext.assetManager->GetTexture(
        "Textures/cerberus/normal.ktx");
    world->AddNode(std::move(CerNode));

    activeWorld = world.get();

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

scene::PerspectiveCamera* WorldManager::GetViewportCamera()
{
    return ViewportCamera;
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
