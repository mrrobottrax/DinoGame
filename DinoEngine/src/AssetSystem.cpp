#include "pch.h"

#include "AssetSystem.h"
#include "RenderingSystem.h"

DINO_API IAssetSystem *g_IAssetSystem = &g_AssetSystem;

static Asset_Texture s_DefaultTexture;

void AssetSystem::start() { m_IsInitialized = true; }

void AssetSystem::stop() { m_IsInitialized = false; }

Asset_Texture AssetSystem::load_texture(const char *path) {
  ASSERT(g_RenderingSystem.is_initialized());

  ResourceLoader_arena0_reset();
  ResourceLoader_arena1_reset();

  void *pFile;
  size_t fileSize;
  ASSERT_CODE(
      ResourceLoader_load_file(path, &pFile, &fileSize, ResourceLoader_arena0));

  PngInfo png{};
  ASSERT_CODE(ResourceLoader_decompress_png(pFile, fileSize, &png,
                                            ResourceLoader_arena1));

  ResourceLoader_arena0_reset();

  if (png.InterlaceMethod != 0)
    ASSERT_CODE(ResourceLoader_deinterlace_png(&png, ResourceLoader_arena0));

  ResourceLoader_arena1_reset();

  ASSERT_CODE(ResourceLoader_png_to_rgba8(&png, ResourceLoader_arena1));

  D3D12_CPU_DESCRIPTOR_HANDLE cpu;
  D3D12_GPU_DESCRIPTOR_HANDLE gpu;
  ComPtr<ID3D12Resource> resource = g_RenderingSystem.upload_static_image_rgba(
      png.Width, png.Height, png.Data, &cpu, &gpu);

  ResourceLoader_arena0_reset();
  ResourceLoader_arena1_reset();

  return {
      .Handle = gpu,
      .CpuHandle = cpu,
      .Resource = resource,
      .Width = png.Width,
      .Height = png.Height,
  };
}

Asset_Shader AssetSystem::compile_transparent_quad_shader(
    const char *vertPath, const char *fragPath,
    ID3D12RootSignature *pRootSignature) const {
  ASSERT(g_RenderingSystem.is_initialized());
  ID3D12Device9 *device = g_RenderingSystem.get_device();
  ASSERT(device);

  ResourceLoader_arena0_reset();
  ResourceLoader_arena1_reset();

  void *vsFile;
  size_t vsSize;
  ASSERT_CODE_ALWAYS(ResourceLoader_load_file(vertPath, &vsFile, &vsSize,
                                              ResourceLoader_arena0));

  void *fsFile;
  size_t fsSize;
  ASSERT_CODE_ALWAYS(ResourceLoader_load_file(fragPath, &fsFile, &fsSize,
                                              ResourceLoader_arena1));

  if (!pRootSignature) {
    ComPtr<ID3DBlob> pVSRootSignatureBlob;
    ASSERT_WIN_ALWAYS(D3DGetBlobPart(vsFile, vsSize, D3D_BLOB_ROOT_SIGNATURE, 0,
                                     &pVSRootSignatureBlob));

    ComPtr<ID3DBlob> pPSRootSignatureBlob;
    ASSERT_WIN_ALWAYS(D3DGetBlobPart(fsFile, fsSize, D3D_BLOB_ROOT_SIGNATURE, 0,
                                     &pPSRootSignatureBlob));

    bool rootSignaturesEqual = false;
    if (pVSRootSignatureBlob.Get() && pPSRootSignatureBlob.Get() &&
        pVSRootSignatureBlob->GetBufferSize() ==
            pPSRootSignatureBlob->GetBufferSize()) {
      rootSignaturesEqual =
          (memcmp(pVSRootSignatureBlob->GetBufferPointer(),
                  pPSRootSignatureBlob->GetBufferPointer(),
                  pVSRootSignatureBlob->GetBufferSize()) == 0);
    }

    ASSERT_WIN_ALWAYS(device->CreateRootSignature(
        0, pVSRootSignatureBlob->GetBufferPointer(),
        pVSRootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&pRootSignature)));
  }

  D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{
      .pRootSignature = pRootSignature,
      .VS =
          {
              .pShaderBytecode = vsFile,
              .BytecodeLength = vsSize,
          },
      .PS =
          {
              .pShaderBytecode = fsFile,
              .BytecodeLength = fsSize,
          },
      .DS = {},
      .HS = {},
      .GS = {},
      .StreamOutput =
          {
              .pSODeclaration = nullptr,
              .NumEntries = 0,
              .pBufferStrides = nullptr,
              .NumStrides = 0,
              .RasterizedStream = 0,
          },
      .BlendState =
          {
              .AlphaToCoverageEnable = FALSE,
              .IndependentBlendEnable = FALSE,
              .RenderTarget = {{
                  .BlendEnable = TRUE,
                  .LogicOpEnable = FALSE,
                  .SrcBlend = D3D12_BLEND_SRC_ALPHA,
                  .DestBlend = D3D12_BLEND_INV_SRC_ALPHA,
                  .BlendOp = D3D12_BLEND_OP_ADD,
                  .SrcBlendAlpha = D3D12_BLEND_ONE,
                  .DestBlendAlpha = D3D12_BLEND_ZERO,
                  .BlendOpAlpha = D3D12_BLEND_OP_ADD,
                  .LogicOp = D3D12_LOGIC_OP_NOOP,
                  .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL,
              }},
          },
      .SampleMask = 0xFFFFFFFF,
      .RasterizerState =
          {
              .FillMode = D3D12_FILL_MODE_SOLID,
              .CullMode = D3D12_CULL_MODE_BACK,
              .FrontCounterClockwise = TRUE,
              .DepthBias = 0,
              .DepthBiasClamp = 0,
              .SlopeScaledDepthBias = 0,
              .DepthClipEnable = TRUE,
              .MultisampleEnable = FALSE,
              .AntialiasedLineEnable = FALSE,
              .ForcedSampleCount = 0,
              .ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF,
          },
      .DepthStencilState =
          {
              .DepthEnable = FALSE,
              .DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO,
              .DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS,
              .StencilEnable = FALSE,
              .StencilReadMask = 0,
              .StencilWriteMask = 0,
              .FrontFace = D3D12_STENCIL_OP_KEEP,
              .BackFace = D3D12_STENCIL_OP_KEEP,
          },
      .InputLayout =
          {
              .pInputElementDescs = nullptr,
              .NumElements = 0,
          },
      .IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,
      .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
      .NumRenderTargets = 1,
      .RTVFormats = {DXGI_FORMAT_R8G8B8A8_UNORM},
      .DSVFormat = DXGI_FORMAT_UNKNOWN,
      .SampleDesc =
          {
              .Count = 1,
              .Quality = 0,
          },
      .NodeMask = 0,
      .CachedPSO = {.pCachedBlob = nullptr, .CachedBlobSizeInBytes = 0},
      .Flags = D3D12_PIPELINE_STATE_FLAG_NONE,
  };

  ID3D12PipelineState *pipelineState;
  ASSERT_WIN_ALWAYS(device->CreateGraphicsPipelineState(
      &graphicsPipelineStateDesc, IID_PPV_ARGS(&pipelineState)));

  ResourceLoader_arena0_reset();
  ResourceLoader_arena1_reset();

  return {
      .pPipelineState = pipelineState,
      .pRootSignature = pRootSignature,
  };
}