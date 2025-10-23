#pragma once

#include "UI_Panel.h"
#include "asset_types.h"

constexpr size_t k_UIPixelBasis = 1080;
constexpr float k_UIPixelScale = 1.0f / k_UIPixelBasis;

class DINO_API IUISystem {
public:
  bool is_initialized() const;

  virtual UI_Panel *get_top_panel() = 0;
  virtual float screen_ratio() const = 0;
  virtual float inv_screen_ratio() const = 0;

  // Resets ResourceLoader_arena_0
  // If pRootSignature == nullptr, the root signature is created from the
  // shader.
  virtual Asset_Shader
  compile_transparent_quad_shader(const char *vertPath, const char *fragPath,
                 ID3D12RootSignature *pRootSignature = nullptr) const = 0;

  template <typename T> T *create();
  template <typename T> T *create(UI_Panel *parent);

protected:
  bool m_IsInitialized{};
};

DINO_API extern IUISystem *g_IUISystem;

DINO_API extern Asset_Shader g_UI_RectShader;

inline bool IUISystem::is_initialized() const { return m_IsInitialized; }

template <typename T> T *IUISystem::create() {
  T *t = new T();
  get_top_panel()->add_child(t);
  return t;
}
template <typename T> inline T *ui_create() { return g_IUISystem->create<T>(); }

template <typename T> T *IUISystem::create(UI_Panel *parent) {
  T *t = new T();
  parent->add_child(t);
  return t;
}
template <typename T> inline T *ui_create(UI_Panel *parent) {
  return g_IUISystem->create<T>(parent);
}