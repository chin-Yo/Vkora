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
        Skybox(Skybox&& other) noexcept;

        Skybox& operator=(Skybox&& other) noexcept;
        Skybox();
        Skybox(const std::string& name);

        virtual ~Skybox() = default;

        void GenerateSkybox();

        uint32_t samplesPhi = 180.f; // 用户可调
        uint32_t samplesTheta = 64.f;

        RTTR_ENABLE(Component)
    public:
        ObserverPtr<TextureCube> EnvCube = nullptr;

        // need spawn
        std::unique_ptr<TextureCube> IrradianceMap = nullptr;
        bool bIsIrradianceMapReady = false;
        //std::unique_ptr<ComputePassBase> IrradianceCompute = nullptr;
        std::unique_ptr<TextureCube> SpecularIBLPrefilter = nullptr;
        bool bIsSpecularIBLPrefilterReady = false;
        //std::unique_ptr<ComputePassBase> SpecularCompute = nullptr;
        std::unique_ptr<Texture2D> BRDFLUT = nullptr;
        bool bIsBRDFLUTReady = false;
        //std::unique_ptr<ComputePassBase> BRDFLUTCompute = nullptr;
    private:
        TextureCube* CurrentEnvCube = nullptr;
        void GenerateIBL();
    };
}
