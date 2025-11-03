#include "pch.h"

#include "AssetSystem.h"
#include "IUISystem.h"
#include "RenderingSystem.h"
#include "UI_Image.h"

void UI_Image::add_render_commands(ID3D12GraphicsCommandList10 *pCommandList,
                                   float x, float y, float w, float h) {
  g_RenderingSystem.set_shader(g_UIImageShader, pCommandList);

  mat4_t matrix = mat4_create(x, y, 0, w, h);

  pCommandList->SetGraphicsRoot32BitConstants(0, 16, matrix.Data, 0);
  pCommandList->SetGraphicsRoot32BitConstants(0, 4, Color, 16);

  ID3D12DescriptorHeap *heap = g_RenderingSystem.get_static_descriptor_heap();
  pCommandList->SetDescriptorHeaps(1, &heap);
  pCommandList->SetGraphicsRootDescriptorTable(1, Texture->Handle);

  // pCommandList->DrawInstanced(4, 1, 0, 0);
}