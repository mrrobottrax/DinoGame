#pragma once

struct Asset_Texture {
  D3D12_GPU_DESCRIPTOR_HANDLE Handle{};
  D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle{};
  ID3D12Resource *Resource{};
  uint32_t Width{}, Height{};
};

struct Asset_Shader {
  ID3D12PipelineState *PipelineState{};
  ID3D12RootSignature *RootSignature{};

  void release() {
    PipelineState->Release();
    RootSignature->Release();
  }
};
