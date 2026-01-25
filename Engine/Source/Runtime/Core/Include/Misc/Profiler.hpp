#pragma once
#include <string>
#include <vector>
#include <chrono>
#include <mutex>
#include <unordered_map>

// 定义宏，如果是 Release 版本可以定义为空以消除开销
#ifndef PROFILE_ENABLE
#define PROFILE_ENABLE 1
#endif

// 节点结构：构成树状结构
struct ProfileResult
{
    const char *name;                    // 作用域名称
    double durationMs;                   // 耗时（毫秒）
    std::vector<ProfileResult> children; // 子作用域
};

class Profiler
{
public:
    static Profiler &Get();

    // 开始一帧（通常在每一帧开始时调用）
    void BeginFrame();
    // 结束一帧（交换数据，准备供UI读取）
    void EndFrame();

    // 内部使用：压栈
    void PushScope(const char *name);
    // 内部使用：出栈
    void PopScope();

    // 获取上一帧的完整数据（供 UI 使用）
    const std::vector<ProfileResult> &GetLastFrameResults() const { return m_LastFrameRoot; }

private:
    Profiler() = default;

    // 临时节点结构，用于正在记录中的数据
    struct TempNode
    {
        const char *name;
        std::chrono::high_resolution_clock::time_point startTime;
        std::vector<ProfileResult> children;
    };

    // 这里的逻辑稍微复杂一点：我们需要维护一个“当前正在记录的栈”
    // 当一个Scope结束时，把它转换成 Result 放入父节点的 children 中
    std::vector<TempNode *> m_NodeStack;
    std::vector<ProfileResult> m_CurrentFrameRoot; // 当前帧正在构建的根节点列表
    std::vector<ProfileResult> m_LastFrameRoot;    // 上一帧已完成的数据
};

// RAII 计时器对象
class ScopedTimer
{
public:
    ScopedTimer(const char *name)
    {
#if PROFILE_ENABLE
        Profiler::Get().PushScope(name);
#endif
    }

    ~ScopedTimer()
    {
#if PROFILE_ENABLE
        Profiler::Get().PopScope();
#endif
    }
};

// ==========================================
// 核心宏定义
// ==========================================

#if PROFILE_ENABLE
// 基础宏：手动指定名称
// 使用 ##__LINE__ 确保变量名唯一，防止同一作用域多次使用冲突
#define PROFILE_SCOPE(name) ScopedTimer timer##__LINE__(name)

// 自动宏：使用函数名
#if defined(_MSC_VER)
#define PROFILE_FUNCTION() PROFILE_SCOPE(__FUNCSIG__)
#else
#define PROFILE_FUNCTION() PROFILE_SCOPE(__PRETTY_FUNCTION__)
#endif
#else
#define PROFILE_SCOPE(name)
#define PROFILE_FUNCTION()
#endif