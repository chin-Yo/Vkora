#pragma once
#include "Engine/SceneGraph/Node.hpp"

struct EditorGlobalContextInitInfo
{
    class EditorUIManager* editorUIManager;
};

class EditorGlobalContext
{
public:
    class EditorUIManager* editorUIManager;
    //class EditorSceneManager* m_scene_manager{nullptr};
    //class EditorInputManager* m_input_manager{nullptr};
    //class RenderSystem* m_render_system{nullptr};
    //class WindowSystem* m_window_system{nullptr};
    scene::Node* selectedNode{nullptr};

public:
    void Initialize(const EditorGlobalContextInitInfo& init_info);
    void Clear();
};

extern EditorGlobalContext GEditorGlobalContext;
