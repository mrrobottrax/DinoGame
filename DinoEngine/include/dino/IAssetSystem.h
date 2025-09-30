#pragma once

class GPUImage {
public:
  GPUImage(D3D12_GPU_DESCRIPTOR_HANDLE handle) { m_Handle = handle; }

  D3D12_GPU_DESCRIPTOR_HANDLE get_handle() const { return m_Handle; }

private:
  D3D12_GPU_DESCRIPTOR_HANDLE m_Handle;
};

class DINO_API IAssetSystem {
public:
  virtual GPUImage load_png(const char *path) = 0;
};

DINO_API IAssetSystem *get_asset_system_interface();