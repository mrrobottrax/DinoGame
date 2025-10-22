#include "pch.h"

#include "RenderingSystem.h"
#include "UISystem.h"

DINO_API IUISystem *g_IUISystem = &g_UISystem;

DINO_API Asset_Shader g_UI_RectShader;

void UISystem::start() {
  ASSERT_ALWAYS(g_RenderingSystem.is_initialized());

  //g_UI_RectShader = g_RenderingSystem.(
  //    ASSET_DIR "UI_QuadVertex.cso", ASSET_DIR "UI_QuadPixel", nullptr);

  m_IsInitialized = true;
}

void UISystem::stop() { m_IsInitialized = false; }

void UISystem::add_render_commands(ID3D12GraphicsCommandList10 *pCommandList,
                                   uint32_t w, uint32_t h) {
  m_ScreenDimensions[0] = w;
  m_ScreenDimensions[1] = h;

  m_ScreenRatio = (float)h / w;
  m_InvScreenRatio = (float)w / h;

  UI_Panel *pPanel = get_top_panel();
  pPanel->Dimensions[0] = k_UIPixelBasis * m_InvScreenRatio;
  pPanel->Dimensions[1] = k_UIPixelBasis;

  D3D12_VIEWPORT viewport{
      .TopLeftX = 0,
      .TopLeftY = 0,
      .Width = (FLOAT)w,
      .Height = (FLOAT)h,
      .MinDepth = 0,
      .MaxDepth = 1,
  };
  pCommandList->RSSetViewports(1, &viewport);

  pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

  render_recursive(pPanel, pCommandList, 0, 0, 0, 0);

  g_RenderingSystem.set_shader(Asset_Shader{}, pCommandList);
}

void UISystem::render_recursive(UI_Panel *pPanel,
                                ID3D12GraphicsCommandList10 *pCommandList,
                                float px, float py, float pw, float ph) {
  float x, y, w, h;
  x = pPanel->calc_x(pw);
  y = pPanel->calc_y(ph);
  w = pPanel->calc_w(pw);
  h = pPanel->calc_h(ph);

  float clipX = x * m_ScreenRatio * k_UIPixelScale * 2 + px;
  float clipY = y * k_UIPixelScale * 2 + py;

  float clipW = w * m_ScreenRatio * k_UIPixelScale * 2;
  float clipH = h * k_UIPixelScale * 2;

  pPanel->add_render_commands(pCommandList, clipX, clipY, clipW, clipH);

  uint16_t children = pPanel->get_child_count();
  for (uint16_t i = 0; i < children; ++i) {
    render_recursive(pPanel->get_child(i), pCommandList, x, y, w, h);
  }
}