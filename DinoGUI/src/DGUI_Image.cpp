#include "pch.h"

#include "DGUI_Image.h"
#include "rendering.h"

void DGUI_Image::add_render_commands(ID3D12GraphicsCommandList10 *pCommandList,
                                     float x, float y, float w, float h) {
  ASSERT(m_Parent != nullptr);
  if (!TextureHandle.ptr)
    return;

  float color[4] = {0, 1, 0, 1};
  mat4_t matrix = mat4_create(x, y, Position[2], w, h);

  DGUI_set_shader(&g_TextureShader, pCommandList);
  pCommandList->SetGraphicsRoot32BitConstants(0, 16, matrix.m_Data, 0);
  pCommandList->SetGraphicsRoot32BitConstants(0, 4, color, 16);

  pCommandList->SetDescriptorHeaps(1, &TextureHeap);
  pCommandList->SetGraphicsRootDescriptorTable(1, TextureHandle);

  pCommandList->DrawInstanced(4, 1, 0, 0);
}