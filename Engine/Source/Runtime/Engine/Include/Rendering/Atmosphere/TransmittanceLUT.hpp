#pragma once

#include <memory>
#include <volk.h>

#include "Medium.hpp"
#include "Engine/Texture/Texture2D.hpp"
#include "Framework/Core/Buffer.hpp"


class TransmittanceLUT
{
public:
    TransmittanceLUT(vkb::VulkanDevice* vulkanDevice);
    void Generate(const VkExtent2D& res, const AtmosphereProperties& atmos);

    ~TransmittanceLUT() = default;

private:
    std::unique_ptr<Texture2D> transmittanceLUT;
    vkb::VulkanDevice* device;
    vkb::Buffer* AtmosBuffer = nullptr;
};
