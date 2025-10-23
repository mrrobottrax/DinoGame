#include "pch.h"

#include "UI_Panel.h"

UI_Panel::~UI_Panel() { delete_children(); }

void UI_Panel::add_render_commands(ID3D12GraphicsCommandList10 *pCommandList,
                                   float x, float y, float w, float h) {
  (void)pCommandList;
  (void)x;
  (void)y;
  (void)w;
  (void)h;
}

void UI_Panel::position_children(float w, float h, float pw, float ph) {}

void UI_Panel::add_child(UI_Panel *pPanel) {
  if (m_ChildCount + 1 > m_ChildCapacity) {
    m_ChildCapacity = max(m_ChildCapacity, 1) * 2;

    UI_Panel **pNew =
        (UI_Panel **)realloc(m_Children, m_ChildCapacity * sizeof(UI_Panel *));
    ASSERT_ALWAYS(pNew);

    m_Children = pNew;
  }

  ASSERT(m_Children);
  m_Children[m_ChildCount++] = pPanel;
  pPanel->m_Parent = this;

  ASSERT(m_ChildCount <= 40000);
}

void UI_Panel::delete_children() {
  for (uint32_t i = 0; i < m_ChildCount; ++i) {
    delete m_Children[i];
  }
  free(m_Children);
  m_Children = nullptr;

  m_ChildCapacity = 0;
  m_ChildCount = 0;
}
