#ifndef CNODE_EDITOR_H
#define CNODE_EDITOR_H

#ifdef __cplusplus
#include <imgui.h>
#include <imgui_node_editor.h>

typedef ax::NodeEditor::EditorContext neEditorContext_t;
#else

#include <stdbool.h>
#include <cimgui.h>

typedef struct neEditorContext_s neEditorContext_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

neEditorContext_t*
neCreateEditor(void);

void
neDestroyEditor(neEditorContext_t* editor);

void
neSetCurrentEditor(neEditorContext_t* editor);

void
neBegin(const char* id, const ImVec2 size);

void
neEnd(void);

bool
neShowBackgroundContextMenu(void);

void
neSuspend(void);

void
neResume(void);

void
neBeginNode(int32_t id);

void
neEndNode(void);

void
neBeginPin(int32_t id, bool is_input);

void
neEndPin(void);

bool
neLink(int32_t linkd_id, int32_t from_pin_id, int32_t to_pin_id);

void
neSetNodePosition(int32_t id, ImVec2 pos);

bool
neBeginCreate(void);

bool
neQueryNewLink(int32_t* from_pin, int32_t* to_pin);

bool
neQueryNewNode(int32_t* from_pin);

bool
neAcceptNewItem(ImVec4 color, float thickness);

void
neRejectNewItem(ImVec4 color, float thickness);

void
neEndCreate(void);

bool
neBeginDelete(void);

bool
neQueryDeletedLink(int32_t* link_id);

bool
neQueryDeletedNode(int32_t* node_id);

bool
neAcceptDeletedItem(bool deleteDependencies);

void
neRejectDeletedItem(void);

void
neEndDelete(void);

#ifdef __cplusplus
}
#endif

#endif
