#include "pch.h"

#include "AssetSystem.h"
#include "GameDllSystem.h"
#include "RenderingSystem.h"

DINO_API IAssetSystem *g_IAssetSystem = &g_AssetSystem;

static Asset_Texture s_DefaultTexture;

void AssetSystem::start() {
  ASSERT_ALWAYS(g_GameDllSystem.is_initialized());

  GameInfo &gameInfo = g_GameDllSystem.GameInfo;

  m_TextureCapacity = gameInfo.MaxTextures;
  m_Textures = (AssetContainer<Asset_Texture> *)malloc(
      sizeof(AssetContainer<Asset_Texture>) * m_TextureCapacity);
  for (uint32_t i = 0; i < m_TextureCapacity; ++i) {
    new (&m_Textures[i]) AssetContainer<Asset_Texture>{};
  }

  // load texture to gpu
}

void AssetSystem::stop() {
  free(m_Textures);
  m_Textures = nullptr;
  m_TextureCapacity = 0;
}

HTexture AssetSystem::preload_texture(const char *path) {
  ASSERT_ALWAYS(m_TextureIndex < m_TextureCapacity);

  AssetContainer<Asset_Texture> &container = m_Textures[m_TextureIndex];
  Asset_Texture &asset = container.Asset;

  HTexture hTexture = HTexture{
      .Index = m_TextureIndex,
  };

  ++m_TextureIndex;

  // void *pFile;
  // size_t size;
  // if (!CODE_SUCCESS(ResourceLoader_load_file(path, &pFile, &size,
  //                                            ResourceLoader_arena0))) {
  //   asset.
  // } else {
  // }

  return hTexture;
}

Asset_Texture AssetSystem::get_texture(HTexture hTexture) {
  return Asset_Texture{};
}
