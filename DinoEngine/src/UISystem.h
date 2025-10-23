#pragma once

#include "IUISystem.h"
#include "UI_Panel.h"

class UISystem : public IUISystem {
public:
  void start();
  void stop();

  virtual UI_Panel *get_top_panel() override;
  virtual float screen_ratio() const override;
  virtual float inv_screen_ratio() const override;
  virtual Asset_Shader
  compile_transparent_quad_shader(const char *vertPath, const char *fragPath,
                 ID3D12RootSignature *pRootSignature = nullptr) const override;

  void add_render_commands(ID3D12GraphicsCommandList10 *pCommandList,
                           uint32_t w, uint32_t h);

private:
  UI_Panel m_TopPanel{};

  float m_ScreenRatio{}, m_InvScreenRatio{};

  uint32_t m_ScreenDimensions[2]{};

  ID3D12PipelineState *m_pCurrentPipelineState{};
  ID3D12RootSignature *m_pCurrentRootSignature{};

  void render_recursive(UI_Panel *pPanel,
                        ID3D12GraphicsCommandList10 *pCommandList, float x,
                        float y, float w, float h);
};

inline UISystem g_UISystem{};

inline UI_Panel *UISystem::get_top_panel() {
  ASSERT(is_initialized());
  return &m_TopPanel;
}

inline float UISystem::screen_ratio() const { return m_ScreenRatio; }
inline float UISystem::inv_screen_ratio() const { return m_InvScreenRatio; }
