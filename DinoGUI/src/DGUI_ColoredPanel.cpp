#include "pch.h"

#include "DGUI_ColoredPanel.h"
#include "screen.h"

void DGUI_ColoredPanel::add_render_commands(
    ID3D12GraphicsCommandList10 *pCommandList, unsigned int x, unsigned int y) {
  float matrix[4][4] = {
      {(float)m_Dimensions[0] / g_ScreenDimensions[0] * 2, 0, 0, 0},
      {0, (float)m_Dimensions[1] / g_ScreenDimensions[1] * 2, 0, 0},
      {0, 0, 1, 0},
      {-1 + (float)(m_Position[0] + x) / g_ScreenDimensions[0] * 2,
       -1 + (float)(m_Position[1] + y) / g_ScreenDimensions[1] * 2, 0, 1},
  };
  pCommandList->SetGraphicsRoot32BitConstants(0, 16, matrix, 0);

  pCommandList->SetGraphicsRoot32BitConstants(0, 4, m_Color, 16);

  pCommandList->DrawInstanced(4, 1, 0, 0);
}