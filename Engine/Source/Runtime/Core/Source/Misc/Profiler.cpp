#include "Misc/Profiler.hpp"

#include <algorithm>

Profiler& Profiler::Get()
{
    static Profiler instance;
    return instance;
}

void Profiler::BeginFrame()
{
    // 清空当前帧数据
    m_CurrentFrameRoot.clear();
    // 确保栈是空的（处理异常情况）
    while (!m_NodeStack.empty())
        m_NodeStack.pop_back();
}

void Profiler::EndFrame()
{
    // 帧结束时，将当前帧数据移动到 LastFrameRoot，供UI在下一帧渲染时读取
    // 这样避免了读写冲突（简单的双缓冲思想）
    m_LastFrameRoot = m_CurrentFrameRoot;
}

void Profiler::PushScope(const char* name)
{
    // 创建一个新的临时节点
    TempNode* newNode = new TempNode();
    newNode->name = name;
    newNode->startTime = std::chrono::high_resolution_clock::now();

    m_NodeStack.push_back(newNode);
}

void Profiler::PopScope()
{
    if (m_NodeStack.empty())
        return;

    // 取出栈顶
    TempNode* topNode = m_NodeStack.back();
    m_NodeStack.pop_back();

    auto endTime = std::chrono::high_resolution_clock::now();
    double duration = std::chrono::duration<double, std::milli>(endTime - topNode->startTime).count();

    // 构建结果
    ProfileResult result;
    result.name = topNode->name;
    result.durationMs = duration;
    result.children = std::move(topNode->children); // 接管子节点

    // 如果栈还不为空，说明它是父节点的子节点
    if (!m_NodeStack.empty())
    {
        m_NodeStack.back()->children.push_back(result);
    }
    // 如果栈空了，说明它是根节点之一
    else
    {
        m_CurrentFrameRoot.push_back(result);
    }

    delete topNode;
}
