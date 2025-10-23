#include "pch.h"

#include "UI_Grid.h"

struct UI_Grid::GridLine {
  float Basis;
  UI_GridFlags flags;
};

void UI_Grid::v_split(float basis, UI_GridFlags flags) {
  if (m_vLineCount + 1 > m_vLineCapacity) {
    m_vLineCapacity = max(m_vLineCapacity, 1) * 2;

    GridLine *pNew =
        (GridLine *)realloc(m_vLines, m_vLineCapacity * sizeof(GridLine));
    ASSERT_ALWAYS(pNew);

    m_vLines = pNew;
  }

  ASSERT(m_vLines);
  m_vLines[m_vLineCount++] = {
      .Basis = basis,
      .flags = flags,
  };

  ASSERT(m_vLineCount <= 200);
}

void UI_Grid::h_split(float basis, UI_GridFlags flags) {
  if (m_vLineCount + 1 > m_hLineCapacity) {
    m_hLineCapacity = max(m_hLineCapacity, 1) * 2;

    GridLine *pNew =
        (GridLine *)realloc(m_hLines, m_hLineCapacity * sizeof(GridLine));
    ASSERT_ALWAYS(pNew);

    m_hLines = pNew;
  }

  ASSERT(m_hLines);
  m_hLines[m_hLineCount++] = {
      .Basis = basis,
      .flags = flags,
  };

  ASSERT(m_hLineCount <= 200);
}

UI_Grid::~UI_Grid() {
  free(m_hLines);
  free(m_vLines);

  m_hLines = nullptr;
  m_vLines = nullptr;

  m_hLineCapacity = 0;
  m_vLineCapacity = 0;

  m_hLineCount = 0;
  m_vLineCount = 0;
}

void UI_Grid::position_children() {
  const uint16_t childCount = get_child_count();
  uint16_t iChild = 0;

  // float rowHeight = calc_h();
  for (uint8_t r = 0; r < m_hLineCount + 1; ++r) {
    for (uint8_t c = 0; c < m_vLineCount + 1; ++c) {
      if (iChild >= childCount)
        return;

      UI_Panel *pChild = get_child(iChild);
      pChild->Flags = 0;
      pChild->set_position(0, 0);
      pChild->set_dimensions(0, 0);
      pChild->set_anchor(0, 0);
      pChild->set_pivot(0, 0);

      // pChild->

      ++iChild;
    }
  }
}
