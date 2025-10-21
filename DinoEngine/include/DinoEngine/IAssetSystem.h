#pragma once

#include "asset_types.h"

enum EAssetScope {
  ASSET_SCOPE_STATIC, // Asset is always loaded
  ASSET_SCOPE_LEVEL   // Asset may be unloaded on scene change
};

class DINO_API IAssetSystem {
public:
  bool is_initialized() const { return m_Initialized; }

  virtual void get_default_quad_state_desc(
      D3D12_GRAPHICS_PIPELINE_STATE_DESC *pStateDesc) = 0;

  virtual uint8_t *load_raw(const char *path,
                            EAssetScope scope = ASSET_SCOPE_LEVEL) = 0;
  virtual Asset_Texture load_png(const char *path, bool rawTexture = false,
                                 EAssetScope scope = ASSET_SCOPE_LEVEL) = 0;

  virtual Asset_Shader
  load_shader(const char *vertexPath, const char *fragmentPath,
              D3D12_GRAPHICS_PIPELINE_STATE_DESC *pState,
              ID3D12RootSignature *pRootSignature = nullptr,
              EAssetScope scope = ASSET_SCOPE_LEVEL) = 0;

protected:
  bool m_Initialized;
};

DINO_API extern IAssetSystem *g_IAssetSystem;