#include "pch.h"

#include "DGUI_ColoredPanel.h"
#include "screen.h"

void DGUI_ColoredPanel::add_render_commands(
    ID3D12GraphicsCommandList10 *pCommandList, float startX, float startY) {
  ASSERT(m_Parent != nullptr);

  const float anchorX = Anchor[0] * m_Parent->Dimensions[0];
  const float anchorY = Anchor[1] * m_Parent->Dimensions[1];
  float x =
      (Position[0] + anchorX) * g_ScreenRatio * DGUI_2PIXEL_SCALE + startX;
  float y = (Position[1] + anchorY) * DGUI_2PIXEL_SCALE + startY;

  float w1;
  float h1;

  if (Flags & DGUI_PANEL_FLAG_SUBTRACTIVE_SIZE_X) {
    w1 = m_Parent->Dimensions[0] - Dimensions[0];
  } else {
    w1 = Dimensions[0];
  }

  if (Flags & DGUI_PANEL_FLAG_SUBTRACTIVE_SIZE_Y) {
    h1 = m_Parent->Dimensions[1] - Dimensions[1];
  } else {
    h1 = Dimensions[1];
  }

  float w = w1 * g_ScreenRatio * DGUI_2PIXEL_SCALE;
  float h = h1 * DGUI_2PIXEL_SCALE;

  mat4_t matrix = mat4_create(x, y, Position[2], w, h);

  pCommandList->SetGraphicsRoot32BitConstants(0, 16, matrix.m_Data, 0);
  pCommandList->SetGraphicsRoot32BitConstants(0, 4, m_Color, 16);

  pCommandList->DrawInstanced(4, 1, 0, 0);
}