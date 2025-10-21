#pragma once

#include "asset_types.h"

enum EAssetScope {
  ASSET_SCOPE_GLOBAL, // Asset is global
  ASSET_SCOPE_LEVEL   // Asset may be unloaded on scene change
};

class DINO_API IAssetSystem {
public:
  // load asset types into memory
  virtual HAsset_Binary load_raw(const char *path,
                                 EAssetScope scope = ASSET_SCOPE_LEVEL) = 0;
  virtual HAsset_Texture load_png(const char *path, bool rawTexture = false,
                                  EAssetScope scope = ASSET_SCOPE_LEVEL) = 0;
  virtual HAsset_Shader load_shader(const char *path,
                                    EAssetScope scope = ASSET_SCOPE_LEVEL) = 0;

  // get asset data from handle
  // NOTE: Asset pointers are volatile and may change on defragmentation.
  // Defragmentation can occur on level change or when a new asset is loaded.
  virtual void const *get_data(HAsset_Binary hAsset) = 0;
  virtual Asset_TextureData const *get_texture_data(HAsset_Texture hAsset) = 0;
  virtual Asset_ShaderData const *get_shader_data(HAsset_Shader hAsset) = 0;
};

DINO_API extern IAssetSystem *g_IAssetSystem;