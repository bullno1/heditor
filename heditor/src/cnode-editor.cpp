#include "cnode-editor.h"
#include <imgui_node_editor.h>

namespace ed = ax::NodeEditor;
typedef ed::EditorContext neEditorContext_t;

enum IdKind {
	Node = 1,  // So that the id will never be 0
	Pin = 2,
	Link = 3,
};

template<typename T>
IdKind IdKindOf();

template<>
IdKind IdKindOf<ed::NodeId>() {
	return IdKind::Node;
}

template<>
IdKind IdKindOf<ed::PinId>() {
	return IdKind::Pin;
}

template<>
IdKind IdKindOf<ed::LinkId>() {
	return IdKind::Link;
}

template<typename IdT>
IdT IntToId(int32_t num) {
	return IdT(((uintptr_t)num << 2) | (uintptr_t)IdKindOf<IdT>());
}

template<typename IdT>
int32_t IdToInt(IdT id) {
	return id.Get() >> 2;
}

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
	ed::BeginNode(IntToId<ed::NodeId>(id));
}

void
neEndNode() {
	ed::EndNode();
}

void
neBeginPin(int32_t id, bool is_input) {
	ed::BeginPin(
		IntToId<ed::PinId>(id),
		is_input ? ed::PinKind::Input : ed::PinKind::Output
	);
}

void
neEndPin() {
	ed::EndPin();
}

void
neSetNodePosition(int32_t id, ImVec2 pos) {
	ed::SetNodePosition(IntToId<ed::NodeId>(id), pos);
}

bool
neBeginDelete(void) {
	return ed::BeginDelete();
}

bool
neQueryDeletedLink(int32_t* link_id) {
	ed::LinkId id;
	bool result = ed::QueryDeletedLink(&id);
	*link_id = IdToInt(id);
	return result;
}

bool
neQueryDeletedNode(int32_t* node_id) {
	ed::NodeId id;
	bool result = ed::QueryDeletedNode(&id);
	*node_id = IdToInt(id);
	return result;
}

bool
neAcceptDeletedItem(bool deleteDependencies) {
	return ed::AcceptDeletedItem(deleteDependencies);
}

void
neRejectDeletedItem(void) {
	return ed::RejectDeletedItem();
}

void
neEndDelete(void) {
	ed::EndDelete();
}

}
