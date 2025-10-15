#pragma once

class GPUImage {
public:
  GPUImage(D3D12_GPU_DESCRIPTOR_HANDLE handle, ID3D12DescriptorHeap *heap) {
    m_Handle = handle;
    m_Heap = heap;
  }

  D3D12_GPU_DESCRIPTOR_HANDLE get_handle() const { return m_Handle; }
  ID3D12DescriptorHeap *get_heap() const { return m_Heap; }

private:
  D3D12_GPU_DESCRIPTOR_HANDLE m_Handle;
  ID3D12DescriptorHeap *m_Heap;
};

class DINO_API IAssetSystem {
public:
  virtual GPUImage load_png(const char *path, bool raw = false) = 0;
};

DINO_API IAssetSystem *get_asset_system_interface();