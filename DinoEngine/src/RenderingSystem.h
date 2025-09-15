#pragma once

constexpr UINT k_FramesInFlight = 3;

class RenderingSystem {
  ComPtr<ID3D12Device9> m_pDevice;
  ComPtr<ID3D12CommandQueue> m_pCommandQueue;
  ComPtr<IDXGISwapChain4> m_pSwapChain;
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

private:
  void create_device(IDXGIFactory6 *pDxgiFactory);
};

inline RenderingSystem g_RenderingSystem{};