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

void
neSetNodePosition(int32_t id, ImVec2 pos);

#ifdef __cplusplus
}
#endif

#endif
