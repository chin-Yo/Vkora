#include "LaunchEngineLoop.hpp"

EngineLoop GEngineLoop;

/** 
 * PreInits the engine loop 
 */
int EnginePreInit( const wchar_t* CmdLine )
{
    int ErrorLevel = GEngineLoop.PreInit( CmdLine );

    return( ErrorLevel );
}

/** 
 * Inits the engine loop 
 */
int EngineInit()
{
    int ErrorLevel = GEngineLoop.Init();

    return( ErrorLevel );
}

/** 
 * Ticks the engine loop 
 */
void EngineTick( void )
{
    GEngineLoop.Tick();
}

/**
 * Shuts down the engine
 */
void EngineExit( void )
{
    GEngineLoop.Exit();
}

int GuardedMain(const wchar_t* CmdLine)
{
    EnginePreInit(CmdLine);
    EngineInit();
    EngineTick();
    EngineExit();
    return 0;
}
