#include "pch.h"

#include "DGUI_ColoredPanel.h"
#include "screen.h"

void DGUI_ColoredPanel::add_render_commands(
    ID3D12GraphicsCommandList10 *pCommandList, float baseX, float baseY) {
  ASSERT(m_Parent != nullptr);

  mat4_t matrix = get_matrix(baseX, baseY);

  pCommandList->SetGraphicsRoot32BitConstants(0, 16, matrix.m_Data, 0);
  pCommandList->SetGraphicsRoot32BitConstants(0, 4, m_Color, 16);

  pCommandList->DrawInstanced(4, 1, 0, 0);
}