#include "pch.h"

#include "DGUI_Panel.h"
#include "screen.h"

DGUI_Panel::~DGUI_Panel() { clear_children(); }

void DGUI_Panel::add_render_commands(ID3D12GraphicsCommandList10 *pCommandList,
                                     unsigned int x, unsigned int y) {
  float matrix[4][4] = {
      {(float)m_Dimensions[0] / g_ScreenDimensions[0] * 2, 0, 0, 0},
      {0, (float)m_Dimensions[1] / g_ScreenDimensions[1] * 2, 0, 0},
      {0, 0, 1, 0},
      {-1 + (float)(m_Position[0] + x) / g_ScreenDimensions[0] * 2,
       -1 + (float)(m_Position[1] + y) / g_ScreenDimensions[1] * 2, 0, 1},
  };
  pCommandList->SetGraphicsRoot32BitConstants(0, 16, matrix, 0);
  pCommandList->DrawInstanced(4, 1, 0, 0);
}

void DGUI_Panel::add_child(DGUI_Panel *pPanel) {
  if (m_ChildCount + 1 > m_ChildCapacity) {
    if (m_ChildCapacity == 0) {
      m_ChildCapacity = 1;
    } else {
      m_ChildCapacity *= 2;
    }

    DGUI_Panel **pNew = (DGUI_Panel **)realloc(
        m_Children, m_ChildCapacity * sizeof(DGUI_Panel *));

    ASSERT(pNew);

    m_Children = pNew;
  }

  ASSERT(m_Children);
  m_Children[m_ChildCount++] = pPanel;
}

void DGUI_Panel::clear_children() {
  for (uint32_t i = 0; i < m_ChildCount; ++i) {
    delete m_Children[i];
  }
  free(m_Children);
  m_Children = nullptr;

  m_ChildCapacity = 0;
  m_ChildCount = 0;
}

void DGUI_Panel::set_position(unsigned int x, unsigned int y) {
  m_Position[0] = x;
  m_Position[1] = y;
}

void DGUI_Panel::set_size(unsigned int w, unsigned int h) {
  m_Dimensions[0] = w;
  m_Dimensions[1] = h;
}
