#pragma once

struct Asset_Texture {
  D3D12_GPU_DESCRIPTOR_HANDLE Handle{};
  D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle{};
  Microsoft::WRL::ComPtr<ID3D12Resource> Resource{};
  uint32_t Width{}, Height{};
};

struct Asset_Shader {
  ID3D12PipelineState *pPipelineState{};
  ID3D12RootSignature *pRootSignature{};

  void release() {
    pPipelineState->Release();
    pRootSignature->Release();
  }
};
