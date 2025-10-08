#include "Engine.hpp"
#include "GlobalContext.hpp"
#include "Logging/Logger.hpp"
#include <iostream>
#include <atomic>
#include <chrono>
#include "Render/RenderSystem.hpp"
#include <algorithm>

const float Engine::FPSAlpha = 1.f / 100;

void Engine::LogicalTick(float DeltaTime)
{
}

bool Engine::RendererTick(float DeltaTime)
{
    GRuntimeGlobalContext.renderSystem->Update(DeltaTime);
    return true;
}

void Engine::CalculateFPS(float DeltaTime)
{
    DeltaTime = std::min<float>(DeltaTime, 0.5f);
    FrameCount++;

    if (FrameCount == 1 || FrameCount % 1000 == 0)
    {
        AverageDuration = DeltaTime;
    }
    else
    {
        AverageDuration = AverageDuration * (1 - FPSAlpha) + DeltaTime * FPSAlpha;
    }
    FPS = static_cast<int>(1.f / AverageDuration);
}

float Engine::CalculateDeltaTime()
{
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - LastTickTimePoint);
    LastTickTimePoint = now;
    return duration.count() / 1000000.0f;
}

void Engine::LimitFPS(float& DeltaTime)
{
    if (MaxFPS <= 0)
        return; // No limit

    // Calculate the target frame time in seconds
    const float targetFrameTime = 1.0f / static_cast<float>(MaxFPS);

    // Calculate actual elapsed time
    auto frameEndTime = std::chrono::steady_clock::now();
    float actualFrameTime = std::chrono::duration_cast<std::chrono::microseconds>(frameEndTime - FrameStartTimePoint).
        count() / 1000000.0f;

    // If frame time is shorter than target, sleep for remaining time
    if (actualFrameTime < targetFrameTime)
    {
        float sleepTime = targetFrameTime - actualFrameTime;
        std::this_thread::sleep_for(std::chrono::microseconds(static_cast<long long>(sleepTime * 1000000)));

        // Update DeltaTime with the adjusted value
        DeltaTime = targetFrameTime;
    }

    // Update start time for next frame
    FrameStartTimePoint = std::chrono::steady_clock::now();
}

void Engine::SetIsIconify(bool bIsIconify)
{
    bIsMinimized = bIsIconify;
}

void Engine::StartEngine(const std::string& ConfigFilePath)
{
    Logger::Init();
    GRuntimeGlobalContext.StartSystems(ConfigFilePath);
    LOG_INFO("Engine started")
}

void Engine::ShutdownEngine()
{
    GRuntimeGlobalContext.ShutdownSystems();
    LOG_INFO("Engine exit")
}

void Engine::Initialize()
{
    ApplicationOptions app_options;
    app_options.benchmark_enabled = false;
    app_options.window = GRuntimeGlobalContext.windowSystem.get();
    GRuntimeGlobalContext.windowSystem->RegisterOnWindowIconifyFunc([this](bool bIsIconify)
        {
            if (this != nullptr)
                this->SetIsIconify(bIsIconify);
        }
    );
    if (!GRuntimeGlobalContext.renderSystem->Prepare(app_options))
    {
        LOG_CRITICAL("Prepare failed !!!")
        std::abort();
    }
}

void Engine::Clear()
{
}

void Engine::Run()
{
}

bool Engine::TickOneFrame(float DeltaTime)
{
    LogicalTick(DeltaTime);
    CalculateFPS(DeltaTime);

    if (!bIsMinimized)
    {
        RendererTick(DeltaTime);
    }

    GRuntimeGlobalContext.windowSystem->ProcessEvents();
    GRuntimeGlobalContext.windowSystem->SetTitle(
        std::string("CyREngine - " + std::to_string(GetFPS()) + " FPS").c_str());
    const bool should_window_close = GRuntimeGlobalContext.windowSystem->ShouldClose();
    if (should_window_close)
    {
        GRuntimeGlobalContext.renderSystem->Finish();
    }
    return !should_window_close;
}

std::string Engine::GetEngineStatus() const
{
    return std::string();
}
