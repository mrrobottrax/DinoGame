#include "pch.h"

#include "AssetSystem.h"
#include "IUISystem.h"
#include "RenderingSystem.h"
#include "UI_Image.h"

void UI_Image::set_texture(const char *path) {
  //m_hTexture = g_AssetSystem.preload_texture(path);
}

void UI_Image::add_render_commands(ID3D12GraphicsCommandList10 *pCommandList,
                                   float x, float y, float w, float h) {
  // Asset_Texture texture = g_AssetSystem.get_texture(m_hTexture);
  // g_RenderingSystem.set_shader(g_UIImageShader, pCommandList);

  // mat4_t matrix = mat4_create(x, y, 0, w, h);

  // pCommandList->SetGraphicsRoot32BitConstants(0, 16, matrix.Data, 0);
  // pCommandList->SetGraphicsRoot32BitConstants(0, 4, Color, 16);

  // pCommandList->SetDescriptorHeaps(1, &texture.Heap);
  // pCommandList->SetGraphicsRootDescriptorTable(0, texture.Handle);

  // pCommandList->DrawInstanced(4, 1, 0, 0);
}