#pragma once

#include "IUISystem.h"
#include "UI_Panel.h"

class UISystem : public IUISystem {
public:
  void start();
  void stop();

  virtual UI_Panel *get_top_panel() override;
  virtual Asset_Shader
  compile_transparent_quad_shader(const char *vertPath, const char *fragPath,
                 ID3D12RootSignature *pRootSignature = nullptr) const override;

  void add_render_commands(ID3D12GraphicsCommandList10 *pCommandList,
                           uint32_t w, uint32_t h);

private:
  UI_Panel m_TopPanel{};

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
