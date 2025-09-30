#include "pch.h"

#include "DGUI_ColoredPanel.h"
#include "rendering.h"
#include "screen.h"

void DGUI_ColoredPanel::add_render_commands(
    ID3D12GraphicsCommandList10 *pCommandList, float x, float y, float w, float h) {
  ASSERT(m_Parent != nullptr);

  mat4_t matrix = mat4_create(x, y, Position[2], w, h);

  dgui_set_shader(&g_RectShader, pCommandList);
  pCommandList->SetGraphicsRoot32BitConstants(0, 16, matrix.m_Data, 0);
  pCommandList->SetGraphicsRoot32BitConstants(0, 4, m_Color, 16);

  pCommandList->DrawInstanced(4, 1, 0, 0);
}