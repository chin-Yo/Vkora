#pragma once
#include <string>
#include <vector>
#include <chrono>
#include <mutex>
#include <unordered_map>

#if BUILD_DEBUG
#define PROFILE_ENABLE 1
#else
#define PROFILE_DISABLE 0
#endif


struct ProfileResult
{
    const char* name;
    double durationMs;
    std::vector<ProfileResult> children;
};

class Profiler
{
public:
    static Profiler& Get();

    // 开始一帧（通常在每一帧开始时调用）
    void BeginFrame();
    // 结束一帧（交换数据，准备供UI读取）
    void EndFrame();

    // 内部使用：压栈
    void PushScope(const char* name);
    // 内部使用：出栈
    void PopScope();

    // 获取上一帧的完整数据（供 UI 使用）
    const std::vector<ProfileResult>& GetLastFrameResults() const { return m_LastFrameRoot; }

private:
    Profiler() = default;

    struct TempNode
    {
        const char* name;
        std::chrono::high_resolution_clock::time_point startTime;
        std::vector<ProfileResult> children;
    };
    std::vector<TempNode*> m_NodeStack;
    std::vector<ProfileResult> m_CurrentFrameRoot;
    std::vector<ProfileResult> m_LastFrameRoot;
};

class ScopedTimer
{
public:
    ScopedTimer(const char* name)
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
#if PROFILE_ENABLE
#define PROFILE_SCOPE(name) ScopedTimer timer##__LINE__(name);

#if defined(_MSC_VER)
#define PROFILE_FUNCTION() PROFILE_SCOPE(__FUNCSIG__)
#else
#define PROFILE_FUNCTION() PROFILE_SCOPE(__PRETTY_FUNCTION__)
#endif
#else
#define PROFILE_SCOPE(name)
#define PROFILE_FUNCTION()
#endif
