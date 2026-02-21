#include "Rendering/Atmosphere/TransmittanceLUT.hpp"

#include "GlobalContext.hpp"
#include "Engine/Asset/Manager/AssetManager.hpp"
#include "Engine/Texture/TextureFactory.hpp"
#include "Framework/Core/ShaderModule.hpp"
#include "Framework/Misc/ResourceBindingState.hpp"
#include "Misc/Paths.hpp"
#include "Rendering/ComputePipeline/ComputePass.hpp"

TransmittanceLUT::TransmittanceLUT(vkb::VulkanDevice* vulkanDevice)
    : device(vulkanDevice)
{
    vkb::BufferBuilder builder(sizeof(AtmosphereProperties));
    builder.with_vma_flags(
               VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT)
           .with_usage(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
           .with_vma_usage(VMA_MEMORY_USAGE_AUTO);

    AtmosBuffer = new vkb::Buffer(*device, builder);
}

void TransmittanceLUT::Generate(const VkExtent2D& res, const AtmosphereProperties& atmos)
{
    transmittanceLUT.reset();
    transmittanceLUT = TextureFactory::CreateTexture2DFromMemory("transmittanceLUT", {}
                                                                 , res.width, res.height,
                                                                 VK_FORMAT_R32G32B32A32_SFLOAT);
    transmittanceLUT->create_vk_image(*device, VK_IMAGE_USAGE_STORAGE_BIT);
    transmittanceLUT->TransitionImageLayout(*device);
    transmittanceLUT->sampler = GRuntimeGlobalContext.assetManager->defaultSampler;
    AtmosBuffer->update(&atmos, sizeof(AtmosphereProperties));
    auto transmittanceSpv = vkb::ShaderSource{Paths::GetShaderFullPath("Atomsphere/transmittance.comp.spv")};
    ComputePassBase* transmittanceLUTCompute = new ComputePassBase{
        *device, std::move(transmittanceSpv)
    };
    transmittanceLUTCompute->Dispatch(res.width / 16, res.height / 16, 1
                                      , [this](vkb::ResourceBindingState& RBS,
                                               std::vector<uint8_t>& stored_push_constants)-> void
                                      {
                                          RBS.bind_buffer(*AtmosBuffer, 0, AtmosBuffer->get_size(), 0, 1, 0);
                                          RBS.bind_image(transmittanceLUT->get_vk_image_view(),
                                                         *transmittanceLUT->sampler.lock(), 0, 0, 0);
                                      });
}
