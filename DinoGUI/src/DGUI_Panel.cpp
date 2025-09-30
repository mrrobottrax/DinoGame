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

float DGUI_Panel::calc_x(float baseX) {
  const float anchorX = Anchor[0] * m_Parent->calc_w();
  float x = (Position[0] + anchorX) * g_ScreenRatio * DGUI_2PIXEL_SCALE + baseX;
  return x;
}

float DGUI_Panel::calc_y(float baseY) {
  const float anchorY = Anchor[1] * m_Parent->calc_h();
  float y = (Position[1] + anchorY) * g_ScreenRatio * DGUI_2PIXEL_SCALE + baseY;
  return y;
}

float DGUI_Panel::calc_w() {
  if (Flags & DGUI_PANEL_FLAG_SUBTRACTIVE_SIZE_X) {
    return m_Parent->Dimensions[0] - Dimensions[0];
  } 
    return Dimensions[0];
}

float DGUI_Panel::calc_h() {
  if (Flags & DGUI_PANEL_FLAG_SUBTRACTIVE_SIZE_Y) {
    return m_Parent->Dimensions[1] - Dimensions[1];
  }
  return Dimensions[1];
}

mat4_t DGUI_Panel::get_matrix(float baseX, float baseY) {
  float x = calc_x(baseX);
  float y = calc_y(baseY);

  float w = calc_w() * g_ScreenRatio * DGUI_2PIXEL_SCALE;
  float h = calc_h() * DGUI_2PIXEL_SCALE;

  return mat4_create(x, y, Position[2], w, h);
}
