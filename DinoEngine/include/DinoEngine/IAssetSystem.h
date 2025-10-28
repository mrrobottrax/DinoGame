#pragma once

#include "asset_types.h"

class DINO_API IAssetSystem {
public:
  bool is_initialized() const;

  virtual HTexture preload_texture(const char *path) = 0;
  virtual Asset_Texture get_texture(HTexture hTexture) = 0;

protected:
  bool m_IsInitialized{};
};

DINO_API extern IAssetSystem *g_IAssetSystem;

inline bool IAssetSystem::is_initialized() const { return m_IsInitialized; }