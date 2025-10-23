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

  float rowHeight;   // Height in pixels
  float columnWidth; // Width in pixels

  GridLine row;
  GridLine column;

  for (uint8_t r = 0; r < m_hLineCount + 1; ++r) {
    if (r == m_hLineCount) {
      row = {
          .Basis = 1,
          .Flags = UI_GRID_FLAG_RELATIVE_SIZE,
      };
    } else {
      row = m_hLines[r];
    }

    if (row.Flags & UI_GRID_FLAG_RELATIVE_SIZE)
      rowHeight = (h * row.Basis + 1) / 2 * k_UIPixelBasis;
    else if (row.Flags & UI_GRID_FLAG_ABSOLUTE_SIZE)
      rowHeight = (row.Basis + 1) / 2 * g_IUISystem->screen_dimensions()[1];
    else
      rowHeight = (row.Basis + 1) / 2 * k_UIPixelBasis;

    for (uint8_t c = 0; c < m_vLineCount + 1; ++c) {
      if (iChild >= childCount)
        return;

      if (c == m_vLineCount) {
        column = {
            .Basis = 1,
            .Flags = UI_GRID_FLAG_RELATIVE_SIZE,
        };
      } else {
        column = m_vLines[r];
      }

      // TODO: Fix
      if (column.Flags & UI_GRID_FLAG_RELATIVE_SIZE)
        columnWidth =
            (w * column.Basis + 1) / 2 * g_IUISystem->screen_dimensions()[0];
      else if (column.Flags & UI_GRID_FLAG_ABSOLUTE_SIZE)
        columnWidth =
            (column.Basis + 1) / 2 * g_IUISystem->screen_dimensions()[0];
      else
        columnWidth = (column.Basis + 1) / 2 * k_UIPixelBasis;

      UI_Panel *pChild = get_child(iChild);
      ASSERT(pChild);

      pChild->Flags = 0;
      pChild->set_position(0, 0);
      pChild->set_dimensions(0, 0);
      pChild->set_anchor(0, 0);
      pChild->set_pivot(0, 0);

      pChild->Dimensions[0] = columnWidth;
      pChild->Dimensions[1] = rowHeight;

      ++iChild;
    }
  }
}
