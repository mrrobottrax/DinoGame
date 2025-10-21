#pragma once

#define DEFINE_ASSET_TYPE(Name)                                                \
  struct Name {                                                                \
    uint32_t m_Index;                                                          \
    uint32_t m_Version;                                                        \
  };

DEFINE_ASSET_TYPE(HAsset_Binary)
DEFINE_ASSET_TYPE(HAsset_Texture)
DEFINE_ASSET_TYPE(HAsset_Shader)

struct Asset_TextureData {
  D3D12_GPU_DESCRIPTOR_HANDLE Handle;
  ID3D12DescriptorHeap *Heap;

  uint32_t Width, Height;
};

struct Asset_ShaderData {
  ID3D12PipelineState *pPipelineState;
  ID3D12RootSignature *pRootSignature;
};

#undef DEFINE_ASSET_TYPE