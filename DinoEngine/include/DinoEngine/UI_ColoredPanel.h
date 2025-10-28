#pragma once

#include "UI_Panel.h"

class DINO_API UI_ColoredPanel : public UI_Panel {
public:
  float Color[4]{1, 1, 1, 1};

  virtual void add_render_commands(ID3D12GraphicsCommandList10 *pCommandList,
                                   float x, float y, float w, float h) override;

  void set_color(float r, float g, float b, float a);
};

inline void UI_ColoredPanel::set_color(float r, float g, float b, float a) {
  Color[0] = r;
  Color[1] = g;
  Color[2] = b;
  Color[3] = a;
}