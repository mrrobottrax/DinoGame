#include "pch.h"

#include "global.h"

#include "DGUI_Panel.h"

static DGUI_Panel s_TopPanel;

void dgui_init() {}

void dgui_stop() { dgui_clear_all(); }

DGUI_Panel *dgui_get_top_panel() { return &s_TopPanel; }

void dgui_clear_all() { s_TopPanel.clear_children(); }

void dgui_add_render_commands(ID3D12GraphicsCommandList *pCommandList,
                              D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle) {}