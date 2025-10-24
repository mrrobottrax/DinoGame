#include "pch.h"

#include "IUISystem.h"
#include "UI_Grid.h"

struct UI_Grid::GridLine {
  float Basis;
  UI_GridFlags_t Flags;
};

void UI_Grid::v_split(float basis, UI_GridFlags_t flags) {
  if (m_hLineCount + 1 > m_hLineCapacity) {
    m_hLineCapacity = max(m_hLineCapacity, 1) * 2;

    GridLine *pNew =
        (GridLine *)realloc(m_hLines, m_hLineCapacity * sizeof(GridLine));
    ASSERT_ALWAYS(pNew);

    m_hLines = pNew;
  }

  ASSERT(m_hLines);
  m_hLines[m_hLineCount++] = {
      .Basis = basis,
      .Flags = flags,
  };

  ASSERT(m_hLineCount <= 200);
}

void UI_Grid::h_split(float basis, UI_GridFlags_t flags) {
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
      .Flags = flags,
  };

  ASSERT(m_vLineCount <= 200);
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

void UI_Grid::position_children(float w, float h, float pw, float ph) {
  const uint16_t childCount = get_child_count();
  uint16_t iChild = 0;

  float rh = 0; // Row height in pixels
  float cw = 0; // Column width in pixels

  float ry = 0;
  float cx;

  float maxH = (k_UIReferenceHeight * 0.5f) * h;
  float maxW =
      (k_UIReferenceHeight * 0.5f) * g_IUISystem->inv_screen_ratio() * w;

  GridLine rLine;
  GridLine cLine;

  for (uint8_t r = 0; r < m_hLineCount + 1; ++r) {
    if (r == m_hLineCount) {
      rLine = {
          .Basis = 1,
          .Flags = UI_GRID_FLAG_RELATIVE_SIZE,
      };
    } else {
      rLine = m_hLines[r];
    }

    if (rLine.Flags & UI_GRID_FLAG_RELATIVE_SIZE)
      rh = 0.5f * k_UIReferenceHeight * h * rLine.Basis;
    else if (rLine.Flags & UI_GRID_FLAG_ABSOLUTE_SIZE)
      rh = k_UIReferenceHeight * rLine.Basis *
           g_IUISystem->inv_screen_dimensions()[1];
    else
      rh = rLine.Basis;

    rh = min(rh, maxH - ry);

    cx = 0;

    for (uint8_t c = 0; c < m_vLineCount + 1; ++c) {
      if (iChild >= childCount)
        return;

      if (c == m_vLineCount) {
        cLine = {
            .Basis = 1,
            .Flags = UI_GRID_FLAG_RELATIVE_SIZE,
        };
      } else {
        cLine = m_vLines[c];
      }

      if (cLine.Flags & UI_GRID_FLAG_RELATIVE_SIZE)
        cw = 0.5f * k_UIReferenceHeight * g_IUISystem->inv_screen_ratio() * w *
             cLine.Basis;
      else if (cLine.Flags & UI_GRID_FLAG_ABSOLUTE_SIZE)
        cw = k_UIReferenceHeight * cLine.Basis *
             g_IUISystem->inv_screen_dimensions()[1];
      else
        cw = rLine.Basis;

      cw = min(cw, maxW - cx);

      UI_Panel *pChild = get_child(iChild);
      ASSERT(pChild);

      pChild->Flags = 0;
      pChild->set_anchor(0, 0);
      pChild->set_pivot(0, 0);

      pChild->set_position(cx, ry);
      pChild->set_dimensions(cw, rh);

      cx += cw;
      ++iChild;
    }

    ry += rh;
  }
}
