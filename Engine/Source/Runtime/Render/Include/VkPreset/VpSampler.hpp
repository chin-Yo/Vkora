#pragma once
#include <volk.h>

namespace vp
{
    class SamplerPreset
    {
    public:
        virtual ~SamplerPreset() = default;
        virtual VkSamplerCreateInfo CreateInfo() const = 0;
    };

    class DefaultSamplerPreset : public SamplerPreset
    {
    public:
        VkSamplerCreateInfo CreateInfo() const override;
    };

    class ShadowMapSamplerPreset : public SamplerPreset
    {
    public:
        explicit ShadowMapSamplerPreset(VkCompareOp op = VK_COMPARE_OP_LESS_OR_EQUAL);

        VkSamplerCreateInfo CreateInfo() const override;

    private:
        VkCompareOp compareOp_;
    };
}
