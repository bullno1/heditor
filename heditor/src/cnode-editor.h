#ifndef CNODE_EDITOR_H
#define CNODE_EDITOR_H

#ifdef __cplusplus
#include <imgui.h>
#include <imgui_node_editor.h>

typedef ax::NodeEditor::EditorContext neEditorContext;
typedef ax::NodeEditor::StyleColor neStyleColor;
typedef ax::NodeEditor::StyleVar neStyleVar;

#else

#include <stdbool.h>
#include <cimgui.h>

typedef struct neEditorContext neEditorContext;

typedef enum {
	neStyleColor_Bg,
	neStyleColor_Grid,
	neStyleColor_NodeBg,
	neStyleColor_NodeBorder,
	neStyleColor_HovNodeBorder,
	neStyleColor_SelNodeBorder,
	neStyleColor_NodeSelRect,
	neStyleColor_NodeSelRectBorder,
	neStyleColor_HovLinkBorder,
	neStyleColor_SelLinkBorder,
	neStyleColor_HighlightLinkBorder,
	neStyleColor_LinkSelRect,
	neStyleColor_LinkSelRectBorder,
	neStyleColor_PinRect,
	neStyleColor_PinRectBorder,
	neStyleColor_Flow,
	neStyleColor_FlowMarker,
	neStyleColor_GroupBg,
	neStyleColor_GroupBorder,

	neStyleColor_Count
} neStyleColor ;

typedef enum {
    neStyleVar_NodePadding,
    neStyleVar_NodeRounding,
    neStyleVar_NodeBorderWidth,
    neStyleVar_HoveredNodeBorderWidth,
    neStyleVar_SelectedNodeBorderWidth,
    neStyleVar_PinRounding,
    neStyleVar_PinBorderWidth,
    neStyleVar_LinkStrength,
    neStyleVar_SourceDirection,
    neStyleVar_TargetDirection,
    neStyleVar_ScrollDuration,
    neStyleVar_FlowMarkerDistance,
    neStyleVar_FlowSpeed,
    neStyleVar_FlowDuration,
    neStyleVar_PivotAlignment,
    neStyleVar_PivotSize,
    neStyleVar_PivotScale,
    neStyleVar_PinCorners,
    neStyleVar_PinRadius,
    neStyleVar_PinArrowSize,
    neStyleVar_PinArrowWidth,
    neStyleVar_GroupRounding,
    neStyleVar_GroupBorderWidth,
    neStyleVar_HighlightConnectedLinks,
    neStyleVar_SnapLinkToPinDir,
    neStyleVar_HoveredNodeBorderOffset,
    neStyleVar_SelectedNodeBorderOffset,

    neStyleVar_Count
} neStyleVar;

#endif

typedef struct {
	const char* SettingsFile;
	int DragButtonIndex;
	int SelectButtonIndex;
	int NavigateButtonIndex;
	int ContextMenuButtonIndex;
	bool EnableSmoothZoom;
	float SmoothZoomPower;
} neConfig;

#ifdef __cplusplus
extern "C" {
#endif

neConfig
neConfigDefault(void);

neEditorContext*
neCreateEditor(const neConfig* config);

void
neDestroyEditor(neEditorContext* editor);

void
neSetCurrentEditor(neEditorContext* editor);

void
neBegin(const char* id, const ImVec2 size);

void
nePinRect(ImVec2 a, ImVec2 b);

void
nePinPivotRect(ImVec2 a, const ImVec2 b);

void
nePinPivotAlignment(ImVec2 alignment);

void
neEnd(void);

ImDrawList*
neGetNodeBackgroundDrawList(int32_t id);

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
neAcceptNewItem(void);

bool
neAcceptNewItemEx(ImVec4 color, float thickness);

void
neRejectNewItem(void);

void
neRejectNewItemEx(ImVec4 color, float thickness);

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

void
nePushStyleColor(neStyleColor colorIndex, ImVec4 color);

void
nePopStyleColor(int count);

void
nePushStyleVarFloat(neStyleVar varIndex, float value);

void
nePushStyleVarVec2(neStyleVar varIndex, ImVec2 value);

void
nePushStyleVarVec4(neStyleVar varIndex, ImVec4 value);

void
nePopStyleVar(int count);

void
neGetStyleColor(neStyleColor colorIndex, ImVec4* color);

void
neGetStyleVarFloat(neStyleVar varIndex, float* out);

void
neGetStyleVarVec2(neStyleVar varIndex, ImVec2* out);

void
neGetStyleVarVec4(neStyleVar varIndex, ImVec4* out);

#ifdef __cplusplus
}
#endif

#endif
