#pragma once

#include "IAssetSystem.h"

class AssetSystem : IAssetSystem {
public:
  void init();
  void stop();

  void wipe_level_assets();

  virtual GPUImage load_png(const char *path) override;

private:
  /// <summary>
  /// Stores static per-level buffers (textures mostly)
  /// </summary>
  ComPtr<ID3D12Heap> m_LevelHeap;
  size_t m_LevelHeapOffset;

  /// <summary>
  /// Stores static per-level descriptors
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