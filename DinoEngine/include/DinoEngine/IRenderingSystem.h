#pragma once

#include "asset_types.h"

class IRenderingSystem {
public:
  bool is_initialized() const;

  virtual ID3D12DescriptorHeap *get_static_descriptor_heap() = 0;

  virtual Asset_Shader compile_transparent_quad_shader(
      const char *vertPath, const char *fragPath,
      ID3D12RootSignature *pRootSignature = nullptr,
      DXGI_FORMAT rtvFormat = DXGI_FORMAT_R32G32B32A32_FLOAT) const = 0;

protected:
  bool m_IsInitialized{};
};

DINO_API extern IRenderingSystem *g_IRenderingSystem;

inline bool IRenderingSystem::is_initialized() const { return m_IsInitialized; }