#pragma once

#include "DGUI_Panel.h"

enum DGUI_Image_DisplayMode {
  DGUI_IMAGE_DISPLAY_FIT,
  DGUI_IMAGE_DISPLAY_STRETCH,
};

class DGUI_API DGUI_Image : public DGUI_Panel {
public:
  //HAsset_Texture Texture;
  DGUI_Image_DisplayMode DisplayMode;
  float Color[4] = {1, 1, 1, 1};

  virtual void add_render_commands(ID3D12GraphicsCommandList10 *pCommandList,
                                   float x, float y, float w, float h) override;
};