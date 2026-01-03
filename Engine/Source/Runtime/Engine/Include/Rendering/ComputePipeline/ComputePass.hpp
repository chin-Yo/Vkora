#pragma once
#include <volk.h>
#include <vector>
#include <functional>
#include <deque>
#include <memory>
#include <string>

#include "Framework/Core/ShaderModule.hpp"

namespace vkb
{
    class ResourceBindingState;
}

namespace vkb
{
    class VulkanDevice;
    class CommandBuffer;
    class ComputePipeline;
}

using TaskCallback = std::function<void()>;
using RecordingFunction = std::function<void(vkb::ResourceBindingState&, std::vector<uint8_t>&)>;

struct ComputeTaskContext
{
    VkFence fence{VK_NULL_HANDLE};
    VkCommandBuffer cmdBuffer{VK_NULL_HANDLE};
    TaskCallback onComplete;
};

class ComputePassBase
{
public:
    ComputePassBase(vkb::VulkanDevice& device, vkb::ShaderSource&& compute_shader);
    virtual ~ComputePassBase();

    // Submit a computing task
    // groupCount: Number of thread groups
    // recordFn: User-defined recording logic (including descriptor binding, etc.)
    // callback: Logic to be executed on the CPU after the task is completed
    void Dispatch(uint32_t x, uint32_t y, uint32_t z,
                  RecordingFunction recordFn,
                  TaskCallback callback = nullptr);

    // Call within the main loop to check if the task is completed
    void Poll();
    // disposable
    void SetOnBatchComplete(TaskCallback callback);

protected:
    vkb::VulkanDevice& device;
    vkb::ShaderSource computeShader;
    std::deque<ComputeTaskContext> pendingTasks;
    bool BatchProcessingCompleted = false;
    TaskCallback OnBatchComplete;
};
