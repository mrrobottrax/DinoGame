#pragma once

#include "UI_Panel.h"

enum UI_GridFlags : uint32_t {
  UI_GRID_FLAG_ABSOLUTE_SIZE,
  UI_GRID_FLAG_RELATIVE_SIZE,
};

class DINO_API UI_Grid : public UI_Panel {
public:
  void v_split(float basis, UI_GridFlags flags);
  void h_split(float basis, UI_GridFlags flags);

  virtual ~UI_Grid() override;
  virtual void position_children() override;

private:
  struct GridLine;

  GridLine *m_hLines{};
  GridLine *m_vLines{};

  uint8_t m_hLineCount{};
  uint8_t m_hLineCapacity{};

  uint8_t m_vLineCount{};
  uint8_t m_vLineCapacity{};
};