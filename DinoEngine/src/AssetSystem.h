#pragma once

#include "IAssetSystem.h"

class AssetSystem : IAssetSystem {
public:
  void start();
  void stop();

  void wipe_level_assets();

  virtual HAsset_Binary
  load_raw(const char *path, EAssetScope scope = ASSET_SCOPE_LEVEL) override {
    return HAsset_Binary{};
  }
  virtual HAsset_Texture
  load_png(const char *path, bool rawTexture = false,
           EAssetScope scope = ASSET_SCOPE_LEVEL) override {
    return HAsset_Texture{};
  }
  virtual HAsset_Shader
  load_shader(const char *path,
              EAssetScope scope = ASSET_SCOPE_LEVEL) override {
    return HAsset_Shader{};
  }

  virtual void const *get_data(HAsset_Binary hAsset) override {
    return nullptr;
  }
  virtual Asset_TextureData const *
  get_texture_data(HAsset_Texture hAsset) override {
    return nullptr;
  }
  virtual Asset_ShaderData const *
  get_shader_data(HAsset_Shader hAsset) override {
    return nullptr;
  }

private:
  size_t m_LevelHeapCapacity;
  size_t m_LevelResourceCapacity;

  ComPtr<ID3D12Resource2> m_StagingBuffer;
  size_t m_StagingBufferCapacity;
  unsigned char *m_StagingBufferMap;

  /// <summary>
  /// Stores static per-level buffers (textures and mesh data).
  /// </summary>
  ComPtr<ID3D12Heap> m_LevelHeap;
  size_t m_LevelHeapOffset;

  /// <summary>
  /// Stores static per-level descriptors.
  /// </summary>
  ComPtr<ID3D12DescriptorHeap> m_LevelDescriptorHeap;
  size_t m_LevelDescriptorCount;

  /// <summary>
  /// Stores static per-level resources.
  /// </summary>
  ID3D12Resource2 **m_LevelResources;
  size_t m_LevelResourceCount;
};

inline AssetSystem g_AssetSystem;