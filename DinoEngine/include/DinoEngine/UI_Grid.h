#pragma once

#include "UI_Panel.h"

typedef uint32_t UI_GridFlags_t;

enum UI_GridFlags : UI_GridFlags_t {
  UI_GRID_FLAG_NONE = 0,
  UI_GRID_FLAG_SUBTRACTIVE_SIZE = 1 << 1,
  UI_GRID_FLAG_ABSOLUTE_SIZE = 1 << 2,
  UI_GRID_FLAG_RELATIVE_SIZE = 1 << 3,
};

class DINO_API UI_Grid : public UI_Panel {
public:
  void v_split(float basis, UI_GridFlags_t flags);
  void h_split(float basis, UI_GridFlags_t flags);

  virtual ~UI_Grid() override;
  virtual void position_children(float w, float h, float pw, float ph) override;

private:
  struct GridLine;

  GridLine *m_hLines{};
  GridLine *m_vLines{};

  uint8_t m_hLineCount{};
  uint8_t m_hLineCapacity{};

  uint8_t m_vLineCount{};
  uint8_t m_vLineCapacity{};
};