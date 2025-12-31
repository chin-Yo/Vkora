#include "Engine/SceneGraph/Components/Skybox.hpp"

#include "GlobalContext.hpp"
#include "Engine/Texture/TextureFactory.hpp"
#include "Framework/Misc/ResourceBindingState.hpp"
#include "Misc/Paths.hpp"
#include "Rendering/RenderSystem.hpp"
#include "Constants/MathConstants.hpp"
#include "Engine/Engine.hpp"
#include "Rendering/ComputeSystem.hpp"

namespace scene
{
    Skybox::Skybox(Skybox&& other) noexcept: Component(other.name),
                                             samplesPhi(other.samplesPhi),
                                             samplesTheta(other.samplesTheta),
                                             EnvCube(std::move(other.EnvCube)),
                                             IrradianceMap(std::move(other.IrradianceMap)),
                                             SpecularIBLPrefilter(std::move(other.SpecularIBLPrefilter)),
                                             BRDFLUT(std::move(other.BRDFLUT))
    {
    }

    Skybox& Skybox::operator=(Skybox&& other) noexcept
    {
        if (this == &other)
            return *this;
        name = other.name;
        samplesPhi = other.samplesPhi;
        samplesTheta = other.samplesTheta;
        EnvCube = std::move(other.EnvCube);
        IrradianceMap = std::move(other.IrradianceMap);
        SpecularIBLPrefilter = std::move(other.SpecularIBLPrefilter);
        BRDFLUT = std::move(other.BRDFLUT);
        return *this;
    }

    Skybox::Skybox()
        : Component("Skybox")
    {
    }

    Skybox::Skybox(const std::string& name)
        : Component(name)
    {
    }

    void Skybox::GenerateSkybox()
    {
        if (!EnvCube)
        {
            LOG_WARN(
                "The environment cubemap does not exist, and thus the necessary textures for IBL cannot be generated.")
            return;
        }
        if (EnvCube == CurrentEnvCube)
        {
            LOG_WARN("Please do not generate IBL textures again.")
            return;
        }
        {// Remove existing resources
            IrradianceMap.reset();
            bIsIrradianceMapReady = false;
            SpecularIBLPrefilter.reset();
            bIsSpecularIBLPrefilterReady = false;
            BRDFLUT.reset();
            bIsBRDFLUTReady = false;
        }
        IrradianceMap = TextureFactory::CreateTextureCubeFromMemory(name + "IrradianceMap", {}, 64, 64,
                                                                    VK_FORMAT_R16G16B16A16_SFLOAT);
        IrradianceMap->create_vk_image(GRuntimeGlobalContext.GetDevice(), VK_IMAGE_USAGE_STORAGE_BIT);
        IrradianceMap->TransitionImageLayout(GRuntimeGlobalContext.GetDevice());
        IrradianceMap->sampler = GRuntimeGlobalContext.assetManager->cubeSampler;
        auto Irr = vkb::ShaderSource{Paths::GetShaderFullPath("IBL/IrradianceMapCompute.comp.spv")};

        ComputePassBase* IrradianceCompute = new ComputePassBase{
            GRuntimeGlobalContext.renderSystem->GetDevice(), std::move(Irr)
        };
        IrradianceCompute->Dispatch(4, 4, 6,
                                    [this](vkb::ResourceBindingState& RBS,
                                           std::vector<uint8_t>& stored_push_constants)-> void
                                    {
                                        RBS.bind_image(this->EnvCube->get_vk_image_view(),
                                                       *this->EnvCube->sampler.lock(), 0, 0, 0);
                                        struct pc
                                        {
                                            float deltaPhi;
                                            float deltaTheta;
                                        } push;
                                        push.deltaPhi = (2.0 * M_PI) / float(samplesPhi);
                                        push.deltaTheta = (0.5 * M_PI) / float(samplesTheta);
                                        auto values = vkb::to_bytes(push);

                                        stored_push_constants.insert(stored_push_constants.end(), values.begin(),
                                                                     values.end());

                                        RBS.bind_image(this->IrradianceMap->get_vk_image_view(),
                                                       *this->IrradianceMap->sampler.lock(), 0, 1, 0);
                                        return;
                                    }, [this]()
                                    {
                                        this->IrradianceMap->TransitionImageLayout(
                                            GRuntimeGlobalContext.GetDevice(),
                                            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                                        this->bIsIrradianceMapReady = true;
                                    });
        GEngine->computeSystem->PushPass("IrradianceCompute", IrradianceCompute);

        CurrentEnvCube = EnvCube.get();
    }
}

RTTR_REGISTRATION
{
    using namespace rttr;
    registration::class_<scene::Skybox>("scene::Skybox")
        .constructor<const std::string&>();
}
