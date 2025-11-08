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

  ID3D12Resource *
  upload_static_image_rgba(uint32_t w, uint32_t h, const void *data,
                           D3D12_CPU_DESCRIPTOR_HANDLE *cpuHandle,
                           D3D12_GPU_DESCRIPTOR_HANDLE *gpuHandle);

  virtual void set_shader(Asset_Shader shader,
                          ID3D12GraphicsCommandList10 *pCommandList) override;
  virtual ID3D12DescriptorHeap *get_static_descriptor_heap() override;

  virtual Asset_Shader compile_transparent_quad_shader(
      const char *vertPath, const char *fragPath,
      ID3D12RootSignature *pRootSignature = nullptr) const override;
  virtual Asset_Shader compile_compute_shader(
      const char *path,
      ID3D12RootSignature *pRootSignature = nullptr) const override;

private:
  Asset_Shader m_CurrentShader{};

  ComPtr<ID3D12Device9> m_pDevice{};

  ComPtr<ID3D12Fence1> m_GPUStallFence{};
  UINT m_GPUStallValue{};
  HANDLE m_hGPUStallEvent{};

  ComPtr<ID3D12CommandQueue> m_CommandQueue{};

  size_t m_RtvDescriptorIncrementSize{};
  size_t m_SrvCbvUabDescriptorIncrementSize{};

  // Swapchain
  struct FrameData {
    ComPtr<ID3D12CommandAllocator> CommandAllocator{};
    ComPtr<ID3D12GraphicsCommandList10> CommandList{};
    ComPtr<ID3D12Resource> Backbuffer{};
    ComPtr<ID3D12Resource> RenderTextures[2]{};
    ComPtr<ID3D12Fence1> Fence{};
    UINT FenceValue{};
    HANDLE FenceEvent{};
  };

  struct {
    ComPtr<IDXGISwapChain4> SwapChain{};
    unsigned int Width{}, Height{};

    ComPtr<ID3D12DescriptorHeap> BackBuffer_RTVDescriptorHeap{};
    ComPtr<ID3D12DescriptorHeap> RenderTexture_RTVDescriptorHeap{};
    ComPtr<ID3D12DescriptorHeap> RenderTexture_SRVDescriptorHeap{};
    FrameData FrameData[k_FramesInFlight]{};
  } m_SwapChain;

  // Staging
  ComPtr<ID3D12CommandAllocator> m_pStagingAllocator{};
  ComPtr<ID3D12GraphicsCommandList10> m_pStagingList{};
  ComPtr<ID3D12Fence1> m_pStagingFence{};
  UINT m_StagingFenceValue{};
  HANDLE m_StagingFenceEvent{};

  ComPtr<ID3D12Heap> m_StagingHeap{};
  ComPtr<ID3D12Resource> m_StagingResource{};
  void *m_StagingHeapMap{};
  size_t m_StagingHeapSize{};

  // Assets
  ComPtr<ID3D12DescriptorHeap> m_DescriptorHeap{};
  uint32_t m_DescriptorCapacity{};
  uint32_t m_DescriptorCount{};

  ComPtr<ID3D12Heap> m_DataHeap{};
  size_t m_DataHeapCapacity{};
  size_t m_DataHeapOffset{};

  ID3D12Resource **m_Resources{};
  uint32_t m_ResourceCapacity{};
  uint32_t m_ResourceCount{};

  void create_device(IDXGIFactory6 *pDxgiFactory);
  void create_backbuffer_data();
};

inline RenderingSystem g_RenderingSystem{};

inline ID3D12Device9 *RenderingSystem::get_device() { return m_pDevice.Get(); }

inline ID3D12CommandQueue *RenderingSystem::get_queue() {
  return m_CommandQueue.Get();
}

inline ID3D12DescriptorHeap *RenderingSystem::get_static_descriptor_heap() {
  return m_DescriptorHeap.Get();
}