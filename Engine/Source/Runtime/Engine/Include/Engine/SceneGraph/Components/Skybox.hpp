#pragma once
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
        TextureCube* EnvCube = nullptr;

        // need spawn
        TextureCube* IrradianceMap = nullptr;
        TextureCube* SpecularIBLPrefilter = nullptr;
        Texture2D* BRDFLUT = nullptr;
    };
}
