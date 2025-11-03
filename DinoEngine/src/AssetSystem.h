#pragma once

#include "IAssetSystem.h"

class AssetSystem : public IAssetSystem {
public:
  void start();
  void stop();

  virtual Asset_Texture load_texture(const char *path) override;
  virtual Asset_Shader compile_transparent_quad_shader(
      const char *vertPath, const char *fragPath,
      ID3D12RootSignature *pRootSignature = nullptr) const override;
};

inline AssetSystem g_AssetSystem{};