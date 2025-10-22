#pragma once

#include "UI_Panel.h"
#include "asset_types.h"

enum UI_Image_DisplayMode {
  UI_IMAGE_DISPLAY_FIT,
  UI_IMAGE_DISPLAY_STRETCH,
};

class DINO_API UI_Image : public UI_Panel {
public:
  Asset_Texture Texture;
  UI_Image_DisplayMode DisplayMode;
  float Color[4] = {1, 1, 1, 1};

  virtual void add_render_commands(ID3D12GraphicsCommandList10 *pCommandList,
                                   float x, float y, float w, float h) override;
};