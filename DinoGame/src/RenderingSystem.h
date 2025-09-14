#pragma once

class RenderingSystem {
  ComPtr<ID3D12Device> m_pDevice;

public:
  void init();
  void stop();

private:
  void create_device(ComPtr<IDXGIFactory6> pDxgiFactory);
};

inline RenderingSystem g_RenderingSystem;