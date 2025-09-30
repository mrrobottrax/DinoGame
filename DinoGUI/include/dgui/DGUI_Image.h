#pragma once

#include "DGUI_Panel.h"

class DGUI_API DGUI_Image : public DGUI_Panel {
public:
  virtual void add_render_commands(ID3D12GraphicsCommandList10 *pCommandList,
                                   float x, float y, float w, float h) override;
};