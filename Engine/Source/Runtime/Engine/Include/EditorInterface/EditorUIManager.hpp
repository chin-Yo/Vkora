#pragma once

class EditorUIInterface
{
public:
    EditorUIInterface() = default;
    virtual ~EditorUIInterface() = default;

    virtual void Initialize() = 0;
    virtual void Prepare(VkRenderPass renderPass, VkQueue queue, uint32_t MinImageCount,
                         uint32_t ImageCount) = 0;
    virtual void Shutdown() = 0;

    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;

    virtual void RenderUI() = 0;
};
