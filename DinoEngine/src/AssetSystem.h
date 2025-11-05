#pragma once

#include "IAssetSystem.h"

class AssetSystem : public IAssetSystem {
public:
  void start();
  void stop();

  virtual Asset_Texture load_texture(const char *path) override;
};

inline AssetSystem g_AssetSystem{};