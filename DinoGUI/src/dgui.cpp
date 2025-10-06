#include "pch.h"

#include "dgui.h"

#include "DGUI_Panel.h"
#include "rendering_private.h"

static DGUI_Panel s_TopPanel;

DGUI_API void DGUI_init(ID3D12Device9 *pDevice) { rendering_init(pDevice); }

DGUI_API void DGUI_stop() {
  DGUI_clear_all();
  rendering_stop();
}

DGUI_API DGUI_Panel *DGUI_get_top_panel() { return &s_TopPanel; }

DGUI_API void DGUI_clear_all() { s_TopPanel.delete_children(); }