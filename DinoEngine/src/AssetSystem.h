#pragma once

#include "IAssetSystem.h"

class AssetSystem : IAssetSystem {
public:
  void init();
  void stop();

  void wipe_level_assets();

  virtual GPUImage load_png(const char *path) override;

private:
  size_t m_LevelHeapCapacity;
  size_t m_LevelResourceCapacity;

  ComPtr<ID3D12Resource2> m_StagingBuffer;
  size_t m_StagingBufferCapacity;
  unsigned char *m_StagingBufferMap;

  /// <summary>
  /// Stores static per-level buffers (textures and mesh data)
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