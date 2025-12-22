#pragma once
#include "Core/ObserverPtr.hpp"
#include "Engine/SceneGraph/Component.hpp"
#include "Rendering/ComputePipeline/ComputePass.hpp"

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

        void GenerateSkybox();

        uint32_t samplesPhi = 64; // 用户可调
        uint32_t samplesTheta = 16;

        RTTR_ENABLE(Component)
    public:
        ObserverPtr<TextureCube> EnvCube = nullptr;

        // need spawn
        std::unique_ptr<TextureCube> IrradianceMap = nullptr;
        //std::unique_ptr<ComputePassBase> IrradianceCompute = nullptr;
        std::unique_ptr<TextureCube> SpecularIBLPrefilter = nullptr;
        //std::unique_ptr<ComputePassBase> SpecularCompute = nullptr;
        std::unique_ptr<Texture2D> BRDFLUT = nullptr;
        //std::unique_ptr<ComputePassBase> BRDFLUTCompute = nullptr;
    };
}
