#include "LaunchEngineLoop.hpp"

#include "Engine/Engine.hpp"
#include "Logging/Logger.hpp"

int EngineLoop::PreInit(const wchar_t* CmdLine)
{
    Logger::Init();

    return 0;
}

int EngineLoop::Init()
{
    GEngine = new Engine();
    GEngine->StartEngine("");
    GEngine->Initialize();
    return 0;
}

void EngineLoop::Tick()
{
    GEngine->Tick();
}

void EngineLoop::Exit()
{
    GEngine->Clear();
    GEngine->ShutdownEngine();
    delete GEngine;
}
