#include "cnode-editor.h"
#include "imgui_node_editor.h"

namespace ne = ax::NodeEditor;

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

neConfig
neConfigDefault(void) {
	ne::Config config;
	neConfig cconfig;
	cconfig.SettingsFile = NULL;
	cconfig.DragButtonIndex = config.DragButtonIndex;
	cconfig.SelectButtonIndex = config.DragButtonIndex;
	cconfig.NavigateButtonIndex = config.NavigateButtonIndex;
	cconfig.ContextMenuButtonIndex = config.ContextMenuButtonIndex;
	cconfig.EnableSmoothZoom = config.EnableSmoothZoom;
	cconfig.SmoothZoomPower = config.SmoothZoomPower;
	return cconfig;
}

neEditorContext*
neCreateEditor(const neConfig* cconfig) {
	ne::Config config;
	if (cconfig != NULL) {
		config.SettingsFile = cconfig->SettingsFile;
		config.DragButtonIndex = cconfig->DragButtonIndex;
		config.SelectButtonIndex = cconfig->SelectButtonIndex;
		config.NavigateButtonIndex = cconfig->NavigateButtonIndex;
		config.ContextMenuButtonIndex = cconfig->ContextMenuButtonIndex;
		config.EnableSmoothZoom = cconfig->EnableSmoothZoom;
		config.SmoothZoomPower = cconfig->SmoothZoomPower;
	}

	return ne::CreateEditor(&config);
}

void
neDestroyEditor(neEditorContext* editor) {
	ne::DestroyEditor(editor);
}

void
neSetCurrentEditor(neEditorContext* editor) {
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

ImDrawList*
neGetNodeBackgroundDrawList(int32_t id) {
	return ne::GetNodeBackgroundDrawList(IntToId<ne::NodeId>(id));
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
nePinRect(ImVec2 a, ImVec2 b) {
	ne::PinRect(a, b);
}

void
nePinPivotRect(ImVec2 a, const ImVec2 b) {
	ne::PinPivotRect(a, b);
}

void
nePinPivotAlignment(ImVec2 alignment) {
	ne::PinPivotAlignment(alignment);
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
neLink(int32_t link_id, int32_t from_pin, int32_t to_pin) {
	return ne::Link(
		IntToId<ne::LinkId>(link_id),
		IntToId<ne::PinId>(from_pin),
		IntToId<ne::PinId>(to_pin)
	);
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

bool
neBeginCreate(void) {
	return ne::BeginCreate();
}

bool
neQueryNewLink(int32_t* from_pin, int32_t* to_pin) {
	ne::PinId fromPinId, toPinId;
	bool result = ne::QueryNewLink(&fromPinId, &toPinId);
	*from_pin = IdToInt(fromPinId);
	*to_pin = IdToInt(toPinId);
	return result;
}

bool
neQueryNewNode(int32_t* from_pin) {
	ne::PinId pinId;
	bool result = ne::QueryNewNode(&pinId);
	*from_pin = IdToInt(pinId);
	return result;
}

bool
neAcceptNewItem(ImVec4 color, float thickness) {
	return ne::AcceptNewItem(color, thickness);
}

void
neRejectNewItem(ImVec4 color, float thickness) {
	ne::RejectNewItem(color, thickness);
}

void
neEndCreate(void) {
	ne::EndCreate();
}

void
nePushStyleColor(neStyleColor colorIndex, ImVec4 color) {
	ne::PushStyleColor(colorIndex, color);
}

void
nePopStyleColorN(int count) {
	ne::PopStyleColor(count);
}

void
nePushStyleVarFloat(neStyleVar varIndex, float value) {
	ne::PushStyleVar(varIndex, value);
}

void
nePushStyleVarVec2(neStyleVar varIndex, ImVec2 value) {
	ne::PushStyleVar(varIndex, value);
}

void
nePushStyleVarVec4(neStyleVar varIndex, ImVec4 value) {
	ne::PushStyleVar(varIndex, value);
}

void
nePopStyleVarN(int count) {
	ne::PopStyleVar(count);
}

void
neGetStyleColor(neStyleColor colorIndex, ImVec4* color) {
	*color = ne::GetStyle().Colors[colorIndex];
}

}
