#pragma once

#include "UI_Panel.h"

class DINO_API UI_ColoredPanel : public UI_Panel {
public:
  virtual void add_render_commands(ID3D12GraphicsCommandList10 *pCommandList,
                                   float x, float y, float w, float h) override;

  void set_color(float r, float g, float b, float a) {
    m_Color[0] = r;
    m_Color[1] = g;
    m_Color[2] = b;
    m_Color[3] = a;
  }

private:
  float m_Color[4]{1, 1, 1, 1};
};