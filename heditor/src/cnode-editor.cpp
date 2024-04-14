#include "cnode-editor.h"
#include "imgui_node_editor.h"

namespace ed = ax::NodeEditor;
typedef ed::EditorContext neEditorContext_t;

extern "C" {

neEditorContext_t*
neCreateEditor(void) {
	return ed::CreateEditor();
}

void
neDestroyEditor(neEditorContext_t* editor) {
	ed::DestroyEditor(editor);
}

void
neSetCurrentEditor(neEditorContext_t* editor) {
	ed::SetCurrentEditor(editor);
}

void
neBegin(const char* id, const ImVec2 size) {
	ed::Begin(id, size);
}

void
neEnd(void) {
	ed::End();
}

bool
neShowBackgroundContextMenu(void) {
	return ed::ShowBackgroundContextMenu();
}

void
neSuspend(void) {
	ed::Suspend();
}

void
neResume(void) {
	ed::Resume();
}

void
neBeginNode(int32_t id) {
	ed::BeginNode(ed::NodeId(id));
}

void
neEndNode() {
	ed::EndNode();
}

void
neBeginPin(int32_t id, bool is_input) {
	ed::BeginPin(
		ed::PinId(id),
		is_input ? ed::PinKind::Input : ed::PinKind::Output
	);
}

void
neEndPin() {
	ed::EndPin();
}

void
neSetNodePosition(int32_t id, ImVec2 pos) {
	ed::SetNodePosition(ed::NodeId(id), pos);
}

}
