#pragma once

struct EditorGlobalContextInitInfo
{
    /*class WindowSystem* window_system;
    class RenderSystem* render_system;
    class PiccoloEngine* engine_runtime;*/
};

class EditorGlobalContext
{
public:
    //class EditorSceneManager* m_scene_manager{nullptr};
    //class EditorInputManager* m_input_manager{nullptr};
    //class RenderSystem* m_render_system{nullptr};
    //class WindowSystem* m_window_system{nullptr};

public:
    void Initialize(const EditorGlobalContextInitInfo& init_info);
    void Clear();
};

extern EditorGlobalContext GEditorGlobalContext;
