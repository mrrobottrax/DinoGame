#include "pch.h"

#include "AssetSystem.h"
#include "RenderingSystem.h"
#include "UISystem.h"

DINO_API IUISystem *g_IUISystem = &g_UISystem;

DINO_API Asset_Shader g_UIRectShader;
DINO_API Asset_Shader g_UIImageShader;

void UISystem::start() {
  ASSERT(g_RenderingSystem.is_initialized());

  g_UIRectShader = g_RenderingSystem.compile_transparent_quad_shader(
      "shaders\\DinoEngine\\UI_Quad.vs.cso",
      "shaders\\DinoEngine\\UI_Quad.ps.cso");

  g_UIImageShader = g_RenderingSystem.compile_transparent_quad_shader(
      "shaders\\DinoEngine\\UI_Img.vs.cso",
      "shaders\\DinoEngine\\UI_Img.ps.cso");

  m_IsInitialized = true;
}

void UISystem::stop() {
  m_IsInitialized = false;

  g_UIRectShader.release();
  g_UIImageShader.release();
}

void UISystem::add_render_commands(ID3D12GraphicsCommandList10 *pCommandList,
                                   uint32_t w, uint32_t h) {
  m_ScreenDimensions[0] = w;
  m_ScreenDimensions[1] = h;

  m_InvScreenDimensions[0] = 1.0f / w;
  m_InvScreenDimensions[1] = 1.0f / h;

  m_ScreenRatio = (float)h / w;
  m_InvScreenRatio = (float)w / h;

  UI_Panel *pPanel = get_top_panel();
  pPanel->Dimensions[0] = k_UIReferenceHeight * m_InvScreenRatio;
  pPanel->Dimensions[1] = k_UIReferenceHeight;

  pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

  render_recursive(pPanel, pCommandList, -1, -1, 0, 0);
}

void UISystem::render_recursive(UI_Panel *pPanel,
                                ID3D12GraphicsCommandList10 *pCommandList,
                                float px, float py, float pw, float ph) {
  float x, y, w, h;

  x = pPanel->Position[0];
  y = pPanel->Position[1];

  w = pPanel->Dimensions[0];
  h = pPanel->Dimensions[1];

  // Dimension flags
  if (pPanel->Flags & UI_PANEL_FLAG_RELATIVE_SIZE_W)
    w *= pw;
  else if (pPanel->Flags & UI_PANEL_FLAG_ABSOLUTE_SIZE_W)
    w = w * 2 * m_InvScreenDimensions[0];
  else
    w = w * 2 * m_ScreenRatio * k_UIPixelScale;

  if (pPanel->Flags & UI_PANEL_FLAG_RELATIVE_SIZE_H)
    h *= ph;
  else if (pPanel->Flags & UI_PANEL_FLAG_ABSOLUTE_SIZE_H)
    h = h * 2 * m_InvScreenDimensions[1];
  else
    h = h * 2 * k_UIPixelScale;

  if (pPanel->Flags & UI_PANEL_FLAG_SUBTRACTIVE_SIZE_W)
    w = pw - w;

  if (pPanel->Flags & UI_PANEL_FLAG_SUBTRACTIVE_SIZE_H)
    h = ph - h;

  // Position flags
  if (pPanel->Flags & UI_PANEL_FLAG_RELATIVE_POSITION_X)
    x *= pw;
  else if (pPanel->Flags & UI_PANEL_FLAG_ABSOLUTE_POSITION_X)
    x = x * 2 * m_InvScreenDimensions[0];
  else
    x = x * 2 * m_ScreenRatio * k_UIPixelScale;

  if (pPanel->Flags & UI_PANEL_FLAG_RELATIVE_POSITION_Y)
    y *= ph;
  else if (pPanel->Flags & UI_PANEL_FLAG_ABSOLUTE_POSITION_Y)
    y = y * 2 * m_InvScreenDimensions[1];
  else
    y = y * 2 * k_UIPixelScale;

  // Get final position
  x += px + (pPanel->Anchor[0] * pw) - (pPanel->Pivot[0] * w);
  y += py + (pPanel->Anchor[1] * ph) - (pPanel->Pivot[1] * h);

  // Flip y
  const float y2 = -y - h;

  pPanel->add_render_commands(pCommandList, x, y2, w, h);
  pPanel->position_children(w, h);

  uint16_t children = pPanel->get_child_count();
  for (uint16_t i = 0; i < children; ++i) {
    render_recursive(pPanel->get_child(i), pCommandList, x, y, w, h);
  }
}