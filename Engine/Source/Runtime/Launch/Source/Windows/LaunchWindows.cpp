#include <cstdint>
#include <windows.h>

static int LaunchWindowsStartup(HINSTANCE hInstance, HINSTANCE hPrevInstance, char* pCmdLine, int nCmdShow)
{
    int Error = 0;

    const wchar_t* CmdLineW = nullptr;
    CmdLineW = ::GetCommandLineW();

    extern int32_t GuardedMain(const wchar_t* CmdLine);
    Error = GuardedMain(CmdLineW);
    return Error;
}


#ifdef WITH_CONSOLE
int main(int argc, char* argv[])
{
    HINSTANCE hInst = ::GetModuleHandleW(nullptr);
    return LaunchWindowsStartup(hInst, nullptr, nullptr, 0);
}
#else
#include <windows.h>
int APIENTRY wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int)
{
    return LaunchWindowsStartup(hInst, nullptr, nullptr, 0);
}
#endif


