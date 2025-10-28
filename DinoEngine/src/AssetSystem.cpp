#include "pch.h"

#include "AssetSystem.h"
#include "GameDllSystem.h"

DINO_API IAssetSystem *g_IAssetSystem = &g_AssetSystem;

void AssetSystem::start() {
  GameInfo &gameInfo = g_GameDllSystem.GameInfo;

  m_TextureCapacity = gameInfo.TextureCapacity;
  m_Textures = (AssetContainer<Asset_Texture> *)malloc(
      sizeof(AssetContainer<Asset_Texture>) * m_TextureCapacity);
  for (uint32_t i = 0; i < m_TextureCapacity; ++i) {
    new (&m_Textures[i]) AssetContainer<Asset_Texture>{};
  }
}

void AssetSystem::stop() {
  free(m_Textures);
  m_Textures = nullptr;
  m_TextureCapacity = 0;
}

HTexture AssetSystem::preload_texture(const char *path) {
  void *pFile;
  size_t size;
  if (!CODE_SUCCESS(ResourceLoader_load_file(path, &pFile, &size,
                                             ResourceLoader_arena0))) {
  }

  return HTexture{};
}

Asset_Texture AssetSystem::get_texture(HTexture hTexture) {
  return Asset_Texture{};
}
