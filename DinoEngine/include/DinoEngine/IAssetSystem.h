#pragma once

#include "asset_types.h"

class DINO_API IAssetSystem {
public:
  bool is_initialized() const;

  virtual Asset_Texture load_texture(const char *path) = 0;
  virtual Asset_Shader compile_transparent_quad_shader(
      const char *vertPath, const char *fragPath,
      ID3D12RootSignature *pRootSignature = nullptr) const = 0;

protected:
  bool m_IsInitialized{};
};

DINO_API extern IAssetSystem *g_IAssetSystem;

inline bool IAssetSystem::is_initialized() const { return m_IsInitialized; }