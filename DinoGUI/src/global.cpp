#include "pch.h"

#include "global.h"

void dgui_init() {}

void dgui_stop() { dgui_clear_all(); }

DGUI_Panel *dgui_get_top_panel() { return nullptr; }

void dgui_clear_all() {}

void dgui_add_render_commands(ID3D12GraphicsCommandList *pCommandList,
                              D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle) {
  float color[4] = {0, 1, 0, 1};
  pCommandList->ClearRenderTargetView(rtvHandle, color, 0, nullptr);
}