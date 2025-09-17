#pragma once

constexpr UINT k_FramesInFlight = 3;

class RenderingSystem {
  ComPtr<ID3D12Device9> m_pDevice;
  ComPtr<ID3D12Fence1> m_GPUStallFence;
  UINT m_GPUStallValue;
  HANDLE m_hGPUStallEvent;
  ComPtr<ID3D12CommandQueue> m_pCommandQueue;
  ComPtr<IDXGISwapChain4> m_pSwapChain;
  unsigned int m_SwapChainW, m_SwapChainH;
  ComPtr<ID3D12DescriptorHeap> m_pFrameBufferDescriptorHeap;
  size_t m_RtvDescriptorIncrementSize;

  struct FrameData {
    ComPtr<ID3D12CommandAllocator> commandAllocator;
    ComPtr<ID3D12GraphicsCommandList10> commandList;
    ComPtr<ID3D12Resource2> backbuffer;
    ComPtr<ID3D12Fence1> fence;
    UINT fenceValue;
    HANDLE fenceEvent;
  };
  FrameData m_FrameData[k_FramesInFlight]{};

public:
  void init();
  void stop();
  void frame();
  void try_resize(unsigned int w, unsigned int h);
  void wait_idle();

  ID3D12Device9 *get_device() { return m_pDevice.Get(); }

private:
  void create_device(IDXGIFactory6 *pDxgiFactory);
  void create_backbuffer_data();
};

inline RenderingSystem g_RenderingSystem{};