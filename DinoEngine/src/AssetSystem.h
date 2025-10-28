#pragma once

#include "IAssetSystem.h"

class AssetSystem : public IAssetSystem {
  void start();
  void stop();

  virtual HTexture preload_texture(const char *path) override;
  virtual Asset_Texture get_texture(HTexture hTexture) override;

private:
  template <typename T> struct AssetContainer {
    T Asset{};
    uint32_t Version{};
  };

  uint32_t m_TextureCapacity{};
  AssetContainer<Asset_Texture> *m_Textures{};
};

inline AssetSystem g_AssetSystem{};