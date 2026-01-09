#include <algorithm>

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
        {
            // Remove existing resources
            IrradianceMap.reset();
            bIsIrradianceMapReady = false;
            SpecularIBLPrefilter.reset();
            bIsSpecularIBLPrefilterReady = false;
            BRDFLUT.reset();
            bIsBRDFLUTReady = false;
        }
        GenerateIBL();
        CurrentEnvCube = EnvCube.get();
    }

    void Skybox::GenerateIBL()
    {
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
                                        LOG_INFO("Irradiance map generated")
                                    });
        GEngine->computeSystem->PushPass("IrradianceCompute", IrradianceCompute);

        SpecularIBLPrefilter = TextureFactory::CreateTextureCubeFromMemory(name + "SpecularIBLPrefilter", {}, 512, 512,
                                                                           VK_FORMAT_R16G16B16A16_SFLOAT,
                                                                           std::vector<Texture::Mipmap>(8));
        SpecularIBLPrefilter->create_vk_image(GRuntimeGlobalContext.GetDevice(), VK_IMAGE_USAGE_STORAGE_BIT);
        SpecularIBLPrefilter->CreateMipmapViews();
        SpecularIBLPrefilter->TransitionImageLayout(GRuntimeGlobalContext.GetDevice());
        SpecularIBLPrefilter->sampler = GRuntimeGlobalContext.assetManager->cubeSampler;
        auto Pre = vkb::ShaderSource{Paths::GetShaderFullPath("IBL/SpecularIBLPrefilterCompute.comp.spv")};

        ComputePassBase* PrefilterCompute = new ComputePassBase{
            GRuntimeGlobalContext.renderSystem->GetDevice(), std::move(Pre)
        };

        for (auto it : this->SpecularIBLPrefilter->get_vk_image().get_views())
        {
            float Roughness = (float)it->get_subresource_range().baseMipLevel / 7.f;
            uint32_t Xy = 512 / (it->get_subresource_range().baseMipLevel + 1);
            Xy = Xy / 16;
            Xy = std::max<uint32_t>(Xy, 1);
            PrefilterCompute->Dispatch(Xy, Xy, 6,
                                       [this, Roughness, it](vkb::ResourceBindingState& RBS,
                                                             std::vector<uint8_t>& stored_push_constants)-> void
                                       {
                                           RBS.bind_image(this->EnvCube->get_vk_image_view(),
                                                          *this->EnvCube->sampler.lock(), 0, 0, 0);
                                           struct pc
                                           {
                                               float roughness;
                                               uint32_t numSamples = 32u;
                                           } push;
                                           push.roughness = Roughness;
                                           auto values = vkb::to_bytes(push);

                                           stored_push_constants.insert(stored_push_constants.end(), values.begin(),
                                                                        values.end());

                                           RBS.bind_image(*it, *this->SpecularIBLPrefilter->sampler.lock(), 0, 1, 0);
                                           return;
                                       });
        }
        PrefilterCompute->SetOnBatchComplete([this]()
        {
            this->SpecularIBLPrefilter->TransitionImageLayout(
                GRuntimeGlobalContext.GetDevice(),
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            this->bIsSpecularIBLPrefilterReady = true;
            LOG_INFO("SpecularIBLPrefilter generated")
        });
        GEngine->computeSystem->PushPass("SpecularIBLPrefilterCompute", PrefilterCompute);

        BRDFLUT = TextureFactory::CreateTexture2DFromMemory(name + "BRDFLUT", {}, 512, 512,
                                                            VK_FORMAT_R16G16_SFLOAT);
        BRDFLUT->create_vk_image(GRuntimeGlobalContext.GetDevice(), VK_IMAGE_USAGE_STORAGE_BIT);
        BRDFLUT->TransitionImageLayout(GRuntimeGlobalContext.GetDevice());
        BRDFLUT->sampler = GRuntimeGlobalContext.assetManager->defaultSampler;
        auto BRDF = vkb::ShaderSource{Paths::GetShaderFullPath("IBL/BRDFLUTCompute.comp.spv")};

        ComputePassBase* BRDFLUTCompute = new ComputePassBase{
            GRuntimeGlobalContext.renderSystem->GetDevice(), std::move(BRDF)
        };
        BRDFLUTCompute->Dispatch(32, 32, 1,
                                 [this](vkb::ResourceBindingState& RBS,
                                        std::vector<uint8_t>& stored_push_constants)-> void
                                 {
                                     RBS.bind_image(this->BRDFLUT->get_vk_image_view(),
                                                    *this->BRDFLUT->sampler.lock(), 0, 0, 0);
                                     return;
                                 }, [this]()
                                 {
                                     this->BRDFLUT->TransitionImageLayout(
                                         GRuntimeGlobalContext.GetDevice(),
                                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                                     this->bIsBRDFLUTReady = true;
                                     LOG_INFO("BRDFLUT generated")
                                 });
        GEngine->computeSystem->PushPass("BRDFLUTCompute", BRDFLUTCompute);

        CurrentEnvCube = EnvCube.get();
    }
}

RTTR_REGISTRATION
{
    using namespace rttr;
    registration::class_<scene::Skybox>("scene::Skybox")
        .constructor<const std::string&>();
}
