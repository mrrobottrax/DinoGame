#pragma once

#include "IAssetSystem.h"

class AssetSystem : public IAssetSystem {
public:
  void start();
  void stop();

  void wipe_level_assets();

  virtual void get_default_quad_state_desc(
      D3D12_GRAPHICS_PIPELINE_STATE_DESC *pStateDesc) override;

  virtual uint8_t *load_raw(const char *path,
                            EAssetScope scope = ASSET_SCOPE_LEVEL) override {
    return nullptr;
  }

  virtual Asset_Texture
  load_png(const char *path, bool rawTexture = false,
           EAssetScope scope = ASSET_SCOPE_LEVEL) override {
    return {};
  }

  virtual Asset_Shader
  load_shader(const char *vertexPath, const char *fragmentPath,
              D3D12_GRAPHICS_PIPELINE_STATE_DESC *pState,
              ID3D12RootSignature *pRootSignature = nullptr,
              EAssetScope scope = ASSET_SCOPE_LEVEL) override;

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

  uint32_t m_StaticShaderCapacity;
  uint32_t m_StaticShaderCount;
  uint32_t m_DynamicShaderCapacity;
  uint32_t m_DynamicShaderCount;

  Asset_Shader *m_StaticShaders;
  Asset_Shader *m_DynamicShaders;
};

inline AssetSystem g_AssetSystem;