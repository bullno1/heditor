#ifndef CNODE_EDITOR_H
#define CNODE_EDITOR_H

#ifdef __cplusplus
typedef ax::NodeEditor::EditorContext neEditorContext_t;
#else
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

#ifdef __cplusplus
}
#endif

#endif
