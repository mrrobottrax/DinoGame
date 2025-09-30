#pragma once

#include "DGUI_Panel.h"

class DGUI_API DGUI_ColoredPanel : public DGUI_Panel {
public:
  virtual void add_render_commands(ID3D12GraphicsCommandList10 *pCommandList,
                                   float x, float y) override;

  void set_color(float r, float g, float b, float a) {
    m_Color[0] = r;
    m_Color[1] = g;
    m_Color[2] = b;
    m_Color[3] = a;
  }

protected:
  float m_Color[4]{};
};