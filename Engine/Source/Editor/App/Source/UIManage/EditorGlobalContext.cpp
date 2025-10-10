#include "UIManage/EditorGlobalContext.hpp"
#include "UIManage/EditorUIManager.hpp"

EditorGlobalContext GEditorGlobalContext;

void EditorGlobalContext::Initialize(const EditorGlobalContextInitInfo& init_info)
{
    this->editorUIManager = init_info.editorUIManager;
}

void EditorGlobalContext::Clear()
{
}
