#include "pch.h"

#include "DGUI_Image.h"
#include "rendering.h"
#include "screen.h"

void DGUI_Image::add_render_commands(ID3D12GraphicsCommandList10 *pCommandList,
                                     float x, float y, float w, float h) {
  ASSERT(m_Parent != nullptr);
  if (!TextureHandle.ptr)
    return;

  if (DisplayMode == DGUI_IMAGE_DISPLAY_FIT) {
    float goalAspect = (float)ImageWidth / ImageHeight;
    float aspect = g_InvScreenRatio * (w / h);

    float newW = w;
    float newH = h;
    if (aspect > goalAspect) {
      newW = goalAspect * h * g_ScreenRatio;
    } else {
      newH = w / goalAspect * g_InvScreenRatio;
    }

    float diffX = w - newW;
    float diffY = h - newH;

    x += diffX / 2;
    y += diffY / 2;

    w = newW;
    h = newH;
  }

  mat4_t matrix = mat4_create(x, y, Position[2], w, h);

  DGUI_set_shader(&g_TextureShader, pCommandList);
  pCommandList->SetGraphicsRoot32BitConstants(0, 16, matrix.m_Data, 0);
  pCommandList->SetGraphicsRoot32BitConstants(0, 4, Color, 16);

  pCommandList->SetDescriptorHeaps(1, &TextureHeap);
  pCommandList->SetGraphicsRootDescriptorTable(1, TextureHandle);

  pCommandList->DrawInstanced(4, 1, 0, 0);
}