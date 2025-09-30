#include "pch.h"

#include "DGUI_Image.h"
#include "rendering.h"

void DGUI_Image::add_render_commands(ID3D12GraphicsCommandList10 *pCommandList,
                                     float x, float y, float w, float h) {
  ASSERT(m_Parent != nullptr);

  mat4_t matrix = mat4_create(x, y, Position[2], w, h);

  dgui_set_shader(&g_RectShader, pCommandList);
  pCommandList->SetGraphicsRoot32BitConstants(0, 16, matrix.m_Data, 0);

  float color[4] = {0, 1, 0, 1};
  pCommandList->SetGraphicsRoot32BitConstants(0, 4, color, 16);

  pCommandList->DrawInstanced(4, 1, 0, 0);
}