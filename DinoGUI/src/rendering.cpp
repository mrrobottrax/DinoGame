#include "pch.h"

#include "DGUI_Panel.h"
#include "dgui.h"
#include "rendering.h"
#include "rendering_private.h"
#include "screen.h"

ShaderData *s_pCurrentShader;

DGUI_API ShaderData s_RectShader;

void rendering_init(ID3D12Device9 *pDevice) {
  dgui_compile_shader(pDevice, &s_RectShader,
                      L"dgui_shaders\\DefaultVertex.cso",
                      L"dgui_shaders\\DefaultPixel.cso");
}

void rendering_stop() { dgui_release_shader(&s_RectShader); }

DGUI_API void dgui_compile_shader(ID3D12Device9 *pDevice,
                                  ShaderData *pShaderData,
                                  const wchar_t *vertexPath,
                                  const wchar_t *pixelPath) {
  HANDLE hVSFile = CreateFileW(vertexPath, GENERIC_READ, FILE_SHARE_READ, NULL,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  ASSERT_WIN_EXP_ALWAYS(hVSFile != INVALID_HANDLE_VALUE);

  HANDLE hPSFile = CreateFileW(pixelPath, GENERIC_READ, FILE_SHARE_READ, NULL,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  ASSERT_WIN_EXP_ALWAYS(hPSFile != INVALID_HANDLE_VALUE);

  LARGE_INTEGER liVSFileSize{};
  ASSERT_WIN_EXP_ALWAYS(GetFileSizeEx(hVSFile, &liVSFileSize));

  LARGE_INTEGER liPSFileSize{};
  ASSERT_WIN_EXP_ALWAYS(GetFileSizeEx(hPSFile, &liPSFileSize));

  ASSERT_ALWAYS(liVSFileSize.QuadPart <= DWORD_MAX);
  ASSERT_ALWAYS(liPSFileSize.QuadPart <= DWORD_MAX);

  void *pVSBlob = malloc(liVSFileSize.QuadPart);
  ASSERT_WIN_EXP_ALWAYS(
      ReadFile(hVSFile, pVSBlob, (DWORD)liVSFileSize.QuadPart, NULL, NULL));

  void *pPSBlob = malloc(liPSFileSize.QuadPart);
  ASSERT_WIN_EXP_ALWAYS(
      ReadFile(hPSFile, pPSBlob, (DWORD)liPSFileSize.QuadPart, NULL, NULL));

  CloseHandle(hVSFile);
  CloseHandle(hPSFile);

  ComPtr<ID3DBlob> pVSRootSignatureBlob;
  D3DGetBlobPart(pVSBlob, liVSFileSize.QuadPart, D3D_BLOB_ROOT_SIGNATURE, 0,
                 &pVSRootSignatureBlob);

  ComPtr<ID3DBlob> pPSRootSignatureBlob;
  D3DGetBlobPart(pPSBlob, liPSFileSize.QuadPart, D3D_BLOB_ROOT_SIGNATURE, 0,
                 &pPSRootSignatureBlob);

  bool rootSignaturesEqual = false;
  if (pVSRootSignatureBlob.Get() && pPSRootSignatureBlob.Get() &&
      pVSRootSignatureBlob->GetBufferSize() ==
          pPSRootSignatureBlob->GetBufferSize()) {
    rootSignaturesEqual = (memcmp(pVSRootSignatureBlob->GetBufferPointer(),
                                  pPSRootSignatureBlob->GetBufferPointer(),
                                  pVSRootSignatureBlob->GetBufferSize()) == 0);
  }

  ASSERT_WIN_ALWAYS(
      pDevice->CreateRootSignature(0, pVSRootSignatureBlob->GetBufferPointer(),
                                   pVSRootSignatureBlob->GetBufferSize(),
                                   IID_PPV_ARGS(&pShaderData->pRootSignature)));

  D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{
      .pRootSignature = pShaderData->pRootSignature,
      .VS =
          {
              .pShaderBytecode = pVSBlob,
              .BytecodeLength = (SIZE_T)liVSFileSize.QuadPart,
          },
      .PS =
          {
              .pShaderBytecode = pPSBlob,
              .BytecodeLength = (SIZE_T)liPSFileSize.QuadPart,
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
  ASSERT_WIN_ALWAYS(pDevice->CreateGraphicsPipelineState(
      &graphicsPipelineStateDesc, IID_PPV_ARGS(&pShaderData->pPipelineState)));

  free(pVSBlob);
  free(pPSBlob);
}

DGUI_API void dgui_release_shader(ShaderData *pShaderData) {
  pShaderData->pPipelineState->Release();
  pShaderData->pRootSignature->Release();

  pShaderData->pPipelineState = nullptr;
  pShaderData->pRootSignature = nullptr;
}

DGUI_API void dgui_set_shader(ShaderData *pShaderData,
                              ID3D12GraphicsCommandList10 *pCommandList) {
  if (s_pCurrentShader == pShaderData)
    return;

  ASSERT(pShaderData != nullptr);
  ASSERT(pShaderData->pPipelineState != nullptr);
  ASSERT(pShaderData->pRootSignature != nullptr);

  s_pCurrentShader = pShaderData;
  pCommandList->SetPipelineState(pShaderData->pPipelineState);
  pCommandList->SetGraphicsRootSignature(pShaderData->pRootSignature);
}

static void render_recursive(DGUI_Panel *pPanel,
                             ID3D12GraphicsCommandList10 *pCommandList,
                             unsigned int x, unsigned int y) {
  pPanel->add_render_commands(pCommandList, x, y);

  unsigned int offsets[2];
  offsets[0] = pPanel->get_position_x();
  offsets[1] = pPanel->get_position_y();
  offsets[0] += x;
  offsets[1] += y;
  uint32_t children = pPanel->get_child_count();
  for (uint32_t i = 0; i < children; ++i) {
    render_recursive(pPanel->get_child(i), pCommandList, offsets[0],
                     offsets[1]);
  }
}

DGUI_API void
dgui_add_render_commands(ID3D12GraphicsCommandList10 *pCommandList,
                         unsigned int w, unsigned int h) {
  g_ScreenDimensions[0] = w;
  g_ScreenDimensions[1] = h;

  DGUI_Panel *pPanel = dgui_get_top_panel();

  D3D12_VIEWPORT viewport{
      .TopLeftX = 0,
      .TopLeftY = 0,
      .Width = (FLOAT)w,
      .Height = (FLOAT)h,
      .MinDepth = 0,
      .MaxDepth = 1,
  };
  pCommandList->RSSetViewports(1, &viewport);
  D3D12_RECT scissor{
      .left = 0,
      .top = 0,
      .right = (LONG)w,
      .bottom = (LONG)h,
  };
  pCommandList->RSSetScissorRects(1, &scissor);

  pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

  dgui_set_shader(&s_RectShader, pCommandList);
  render_recursive(pPanel, pCommandList, 0, 0);

  s_pCurrentShader = nullptr;
}