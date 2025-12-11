#pragma once
#include "Core/ObserverPtr.hpp"
#include "Engine/SceneGraph/Component.hpp"

class Texture2D;
class TextureCube;

namespace scene
{
    class Skybox : public Component
    {
    public:
        Skybox(const std::string& name = {});

        virtual ~Skybox() = default;

        //Skybox(Skybox&& other) noexcept;

        //Skybox& operator=(Skybox&& other) noexcept;

        RTTR_ENABLE(Component)
    public:
        ObserverPtr<TextureCube> EnvCube = nullptr;

        // need spawn
        std::unique_ptr<TextureCube> IrradianceMap = nullptr;
        std::unique_ptr<TextureCube> SpecularIBLPrefilter = nullptr;
        std::unique_ptr<Texture2D> BRDFLUT = nullptr;
    };
}
