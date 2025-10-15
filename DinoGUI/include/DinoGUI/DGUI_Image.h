#pragma once

#include "DGUI_Panel.h"

enum DGUI_Image_DisplayMode {
  DGUI_IMAGE_DISPLAY_FIT,
  DGUI_IMAGE_DISPLAY_STRETCH,
};

class DGUI_API DGUI_Image : public DGUI_Panel {
public:
  ID3D12DescriptorHeap *TextureHeap;
  D3D12_GPU_DESCRIPTOR_HANDLE TextureHandle;
  DGUI_Image_DisplayMode DisplayMode;
  uint32_t ImageWidth, ImageHeight;
  float Color[4] = {1, 1, 1, 1};

  virtual void add_render_commands(ID3D12GraphicsCommandList10 *pCommandList,
                                   float x, float y, float w, float h) override;
};