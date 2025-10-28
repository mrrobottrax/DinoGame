#pragma once

struct HAsset {
  uint32_t Index;
};

typedef HAsset HTexture;
typedef HAsset HShader;

struct Asset_Texture {
  D3D12_GPU_DESCRIPTOR_HANDLE Handle{};
  ID3D12DescriptorHeap *Heap{};

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
