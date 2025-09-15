#include "pch.h"

#include "global.h"

#include "DGUI_Panel.h"

static DGUI_Panel s_TopPanel;

void dgui_init() {}

void dgui_stop() { dgui_clear_all(); }

DGUI_Panel *dgui_get_top_panel() { return &s_TopPanel; }

void dgui_clear_all() { s_TopPanel.clear_children(); }

static void render_recursive(DGUI_Panel *pPanel,
                             ID3D12GraphicsCommandList10 *pCommandList) {
  pPanel->add_render_commands(pCommandList);
  uint32_t children = pPanel->get_child_count();
  for (uint32_t i = 0; i < children; ++i) {
    render_recursive(pPanel->get_child(i), pCommandList);
  }
}

void dgui_add_render_commands(ID3D12GraphicsCommandList10 *pCommandList) {
  DGUI_Panel *pPanel = dgui_get_top_panel();
  render_recursive(pPanel, pCommandList);
}