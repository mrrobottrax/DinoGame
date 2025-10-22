#pragma once

#include "IRenderingSystem.h"
#include "asset_types.h"

constexpr UINT k_FramesInFlight = 2;

class RenderingSystem : public IRenderingSystem {
public:
  void start();
  void stop();
  void frame();
  void try_resize(unsigned int w, unsigned int h);
  void wait_idle();

  ID3D12Device9 *get_device();
  ID3D12CommandQueue *get_queue();

  ID3D12GraphicsCommandList10 *reset_staging_list();
  void execute_staging_list();

  virtual void set_shader(Asset_Shader shader,
                  ID3D12GraphicsCommandList10 *pCommandList) override;

private:
  Asset_Shader m_CurrentShader;

  ComPtr<ID3D12Device9> m_pDevice;

  ComPtr<ID3D12Fence1> m_GPUStallFence;
  UINT m_GPUStallValue;
  HANDLE m_hGPUStallEvent;

  ComPtr<ID3D12CommandQueue> m_pCommandQueue;

  ComPtr<IDXGISwapChain4> m_pSwapChain;
  unsigned int m_SwapChainW, m_SwapChainH;

  ComPtr<ID3D12DescriptorHeap> m_pRTVDescriptorHeap;
  size_t m_RtvDescriptorIncrementSize;

  ComPtr<ID3D12CommandAllocator> m_pStagingAllocator;
  ComPtr<ID3D12GraphicsCommandList10> m_pStagingList;
  ComPtr<ID3D12Fence1> m_pStagingFence;
  UINT m_StagingFenceValue;
  HANDLE m_StagingFenceEvent;

  struct FrameData {
    ComPtr<ID3D12CommandAllocator> commandAllocator;
    ComPtr<ID3D12GraphicsCommandList10> commandList;
    ComPtr<ID3D12Resource2> backbuffer;
    ComPtr<ID3D12Fence1> fence;
    UINT fenceValue;
    HANDLE fenceEvent;
  };
  FrameData m_FrameData[k_FramesInFlight]{};

  void create_device(IDXGIFactory6 *pDxgiFactory);
  void create_backbuffer_data();
};

inline RenderingSystem g_RenderingSystem{};

inline ID3D12Device9 *RenderingSystem::get_device() { return m_pDevice.Get(); }

inline ID3D12CommandQueue *RenderingSystem::get_queue() {
  return m_pCommandQueue.Get();
}