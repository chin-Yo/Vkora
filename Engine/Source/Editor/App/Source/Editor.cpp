#include "Editor.hpp"
#include <assert.h>
#include "Engine.hpp"
#include "GlobalContext.hpp"
#include "Render/RenderSystem.hpp"
#include "UIManage/EditorGlobalContext.hpp"
#include "World/WorldManager.hpp"

Editor::Editor()
{
}

Editor::~Editor()
{
}

void Editor::Initialize(Engine* RuntimeEngine)
{
    assert(RuntimeEngine);
    this->RuntimeEngine = RuntimeEngine;

    EditorManager = std::make_unique<EditorUIManager>(GRuntimeGlobalContext.renderSystem->GetDevice());
    EditorManager->Initialize();
    GEditorGlobalContext.Initialize({EditorManager.get()});
}

void Editor::Clear()
{
}

void Editor::Run()
{
    assert(RuntimeEngine);

    float delta_time;
    while (true)
    {
        delta_time = RuntimeEngine->CalculateDeltaTime();
        RuntimeEngine->LimitFPS(delta_time);

        if (!RuntimeEngine->TickOneFrame(delta_time))
            return;
    }
}
