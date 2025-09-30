#include "pch.h"

#include "DGUI_Panel.h"
#include "screen.h"

DGUI_Panel::~DGUI_Panel() { delete_children(); }

void DGUI_Panel::add_render_commands(ID3D12GraphicsCommandList10 *pCommandList,
                                     float x, float y) {
  (void)pCommandList;
  (void)x;
  (void)y;
}

void DGUI_Panel::set_position_dimensions(float x, float y, float w, float h,
                                         float pivotX, float pivotY) {
  Dimensions[0] = w;
  Dimensions[1] = h;

  Position[0] = x - pivotX * w;
  Position[1] = y - pivotY * h;
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
  pPanel->m_Parent = this;
}

void DGUI_Panel::delete_children() {
  for (uint32_t i = 0; i < m_ChildCount; ++i) {
    delete m_Children[i];
  }
  free(m_Children);
  m_Children = nullptr;

  m_ChildCapacity = 0;
  m_ChildCount = 0;
}
