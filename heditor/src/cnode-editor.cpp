#include "cnode-editor.h"
#include <imgui_node_editor.h>

namespace ne = ax::NodeEditor;
typedef ne::EditorContext neEditorContext_t;

enum IdKind {
	Node = 1,  // So that the id will never be 0
	Pin = 2,
	Link = 3,
};

template<typename T>
IdKind IdKindOf();

template<>
IdKind IdKindOf<ne::NodeId>() {
	return IdKind::Node;
}

template<>
IdKind IdKindOf<ne::PinId>() {
	return IdKind::Pin;
}

template<>
IdKind IdKindOf<ne::LinkId>() {
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
	return ne::CreateEditor();
}

void
neDestroyEditor(neEditorContext_t* editor) {
	ne::DestroyEditor(editor);
}

void
neSetCurrentEditor(neEditorContext_t* editor) {
	ne::SetCurrentEditor(editor);
}

void
neBegin(const char* id, const ImVec2 size) {
	ne::Begin(id, size);
}

void
neEnd(void) {
	ne::End();
}

bool
neShowBackgroundContextMenu(void) {
	return ne::ShowBackgroundContextMenu();
}

void
neSuspend(void) {
	ne::Suspend();
}

void
neResume(void) {
	ne::Resume();
}

void
neBeginNode(int32_t id) {
	ne::BeginNode(IntToId<ne::NodeId>(id));
}

void
neEndNode() {
	ne::EndNode();
}

void
neBeginPin(int32_t id, bool is_input) {
	ne::BeginPin(
		IntToId<ne::PinId>(id),
		is_input ? ne::PinKind::Input : ne::PinKind::Output
	);
}

void
neEndPin() {
	ne::EndPin();
}

void
neSetNodePosition(int32_t id, ImVec2 pos) {
	ne::SetNodePosition(IntToId<ne::NodeId>(id), pos);
}

bool
neBeginDelete(void) {
	return ne::BeginDelete();
}

bool
neQueryDeletedLink(int32_t* link_id) {
	ne::LinkId id;
	bool result = ne::QueryDeletedLink(&id);
	*link_id = IdToInt(id);
	return result;
}

bool
neQueryDeletedNode(int32_t* node_id) {
	ne::NodeId id;
	bool result = ne::QueryDeletedNode(&id);
	*node_id = IdToInt(id);
	return result;
}

bool
neAcceptDeletedItem(bool deleteDependencies) {
	return ne::AcceptDeletedItem(deleteDependencies);
}

void
neRejectDeletedItem(void) {
	return ne::RejectDeletedItem();
}

void
neEndDelete(void) {
	ne::EndDelete();
}

}
