#pragma once

class RenderingSystem {
  ComPtr<ID3D12Device9> m_pDevice;
  ComPtr<ID3D12CommandQueue> m_pCommandQueue;
  ComPtr<IDXGISwapChain4> m_pSwapChain;

public:
  void init();
  void stop();

private:
  void create_device(IDXGIFactory6* pDxgiFactory);
};

inline RenderingSystem g_RenderingSystem;