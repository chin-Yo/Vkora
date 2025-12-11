#pragma once

class EngineLoop
{
public:

    int PreInit(const wchar_t* CmdLine);

    int Init();
    
    void Tick();

    void Exit();
};

extern EngineLoop GEngineLoop;