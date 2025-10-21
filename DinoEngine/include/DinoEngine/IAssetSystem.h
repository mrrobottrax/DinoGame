#pragma once

class GPUImage {
public:
  GPUImage(D3D12_GPU_DESCRIPTOR_HANDLE handle, ID3D12DescriptorHeap *heap,
           uint32_t width, uint32_t height) {
    m_Handle = handle;
    m_Heap = heap;
    m_Width = width;
    m_Height = height;
  }

  D3D12_GPU_DESCRIPTOR_HANDLE get_handle() const { return m_Handle; }
  ID3D12DescriptorHeap *get_heap() const { return m_Heap; }

  uint32_t get_width() const { return m_Width; }
  uint32_t get_height() const { return m_Height; }

private:
  D3D12_GPU_DESCRIPTOR_HANDLE m_Handle;
  ID3D12DescriptorHeap *m_Heap;

  uint32_t m_Width, m_Height;
};

class DINO_API IAssetSystem {
public:
  virtual GPUImage load_png(const char *path, bool raw = false) = 0;
};

DINO_API extern IAssetSystem *g_IAssetSystem;