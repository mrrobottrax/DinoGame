#include "pch.h"

#include "DGUI_Panel.h"

DGUI_Panel::~DGUI_Panel() { delete_children(); }

void DGUI_Panel::add_render_commands(ID3D12GraphicsCommandList10 *pCommandList,
                                     float x, float y, float w, float h) {
  (void)pCommandList;
  (void)x;
  (void)y;
  (void)w;
  (void)h;
}

void DGUI_Panel::add_child(DGUI_Panel *pPanel) {
  if (m_ChildCount + 1 > m_ChildCapacity) {
    m_ChildCapacity = max(m_ChildCapacity, 1) * 2;

    DGUI_Panel **pNew = (DGUI_Panel **)realloc(
        m_Children, m_ChildCapacity * sizeof(DGUI_Panel *));
    ASSERT_ALWAYS(pNew);

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
