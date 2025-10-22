#pragma once

#include "asset_types.h"

constexpr size_t k_UIPixelBasis = 1080;
constexpr float k_UIPixelScale = 1.0f / k_UIPixelBasis;

class UI_Panel;

class DINO_API IUISystem {
public:
  bool is_initialized() const;

  virtual UI_Panel *get_top_panel() = 0;
  virtual float screen_ratio() const = 0;
  virtual float inv_screen_ratio() const = 0;

protected:
  bool m_IsInitialized;
};

DINO_API extern IUISystem *g_IUISystem;

DINO_API extern Asset_Shader g_UI_RectShader;

inline bool IUISystem::is_initialized() const { return m_IsInitialized; }