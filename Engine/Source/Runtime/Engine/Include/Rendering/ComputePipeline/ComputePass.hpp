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

// 任务完成后的回调函数签名
using TaskCallback = std::function<void()>;
// 用于录制具体 CommandBuffer 的 lambda (绑定描述符、PushConstants等)
using RecordingFunction = std::function<void(vkb::ResourceBindingState&)>;

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

    // 核心：提交一个计算任务
    // groupCount: 线程组数量
    // recordFn: 用户自定义的录制逻辑（绑定描述符等）
    // callback: 任务完成后在 CPU 端执行的逻辑
    void Dispatch(uint32_t x, uint32_t y, uint32_t z,
                  RecordingFunction recordFn,
                  TaskCallback callback = nullptr);

    // 需要在主循环中调用，检查任务是否完成
    void Poll();

protected:
    vkb::VulkanDevice& device;
    vkb::ShaderSource computeShader;
    // 正在进行的任务队列
    std::deque<ComputeTaskContext> pendingTasks;
};
