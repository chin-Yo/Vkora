#define GLM_ENABLE_EXPERIMENTAL
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <unordered_map>
#include "Engine.hpp"
#include "Editor.hpp"

int main(int argc, char** argv)
{
    std::filesystem::path ExecutablePath(argv[0]);
    std::filesystem::path ConfigFilePath = ExecutablePath.parent_path() / "CyREditor.ini";
    Engine* engine = new Engine();

    engine->StartEngine(ConfigFilePath.generic_string());

    engine->Initialize();

    Editor* editor = new Editor();
    editor->Initialize(engine);

    editor->Run();

    editor->Clear();

    engine->Clear();
    engine->ShutdownEngine();

    return 0;
}
