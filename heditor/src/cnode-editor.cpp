#include "cnode-editor.h"
#include "imgui_node_editor.h"

namespace Editor = ax::NodeEditor;
typedef Editor::EditorContext neEditorContext_t;

extern "C" {

neEditorContext_t*
neCreateEditor(void) {
	return Editor::CreateEditor();
}

void
neDestroyEditor(neEditorContext_t* editor) {
	Editor::DestroyEditor(editor);
}

void
neSetCurrentEditor(neEditorContext_t* editor) {
	Editor::SetCurrentEditor(editor);
}

void
neBegin(const char* id, const ImVec2 size) {
	Editor::Begin(id, size);
}

void
neEnd(void) {
	Editor::End();
}

bool
neShowBackgroundContextMenu(void) {
	return Editor::ShowBackgroundContextMenu();
}

void
neSuspend(void) {
	Editor::Suspend();
}

void
neResume(void) {
	Editor::Resume();
}

}
