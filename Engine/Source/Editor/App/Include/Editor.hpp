#pragma once

#include <memory>

#include "UIManage/EditorUIManager.hpp"

class Engine;

class Editor
{
public:
    Editor();
    virtual ~Editor();

    void Initialize(Engine* RuntimeEngine);
    void Clear();

    void Run();

protected:
    std::unique_ptr<EditorUIManager> EditorManager;
    Engine* RuntimeEngine{nullptr};
};
