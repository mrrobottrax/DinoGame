#pragma once

#include "asset_types.h"

class IRenderingSystem {
public:
  bool is_initialized() const;

  virtual void set_shader(Asset_Shader shader,
                          ID3D12GraphicsCommandList10 *pCommandList) = 0;
  virtual ID3D12DescriptorHeap *get_static_descriptor_heap() = 0;

protected:
  bool m_IsInitialized{};
};

DINO_API extern IRenderingSystem *g_IRenderingSystem;

inline bool IRenderingSystem::is_initialized() const { return m_IsInitialized; }