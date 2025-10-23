#include "pch.h"

#include "RenderingSystem.h"
#include "UISystem.h"

DINO_API IUISystem *g_IUISystem = &g_UISystem;

DINO_API Asset_Shader g_UI_RectShader;

void UISystem::start() {
  ASSERT_ALWAYS(g_RenderingSystem.is_initialized());

  g_UI_RectShader =
      compile_transparent_quad_shader("shaders\\DinoEngine\\UI_QuadVertex.cso",
                                      "shaders\\DinoEngine\\UI_QuadPixel.cso");

  m_IsInitialized = true;
}

void UISystem::stop() {
  m_IsInitialized = false;

  g_UI_RectShader.release();
}

void UISystem::add_render_commands(ID3D12GraphicsCommandList10 *pCommandList,
                                   uint32_t w, uint32_t h) {
  m_ScreenDimensions[0] = w;
  m_ScreenDimensions[1] = h;

  m_InvScreenDimensions[0] = 1.0f / w;
  m_InvScreenDimensions[1] = 1.0f / h;

  m_ScreenRatio = (float)h / w;
  m_InvScreenRatio = (float)w / h;

  UI_Panel *pPanel = get_top_panel();
  pPanel->Dimensions[0] = k_UIPixelBasis * m_InvScreenRatio;
  pPanel->Dimensions[1] = k_UIPixelBasis;

  D3D12_VIEWPORT viewport{
      .TopLeftX = 0,
      .TopLeftY = 0,
      .Width = (FLOAT)w,
      .Height = (FLOAT)h,
      .MinDepth = 0,
      .MaxDepth = 1,
  };
  D3D12_RECT scissor{
      .left = 0,
      .top = 0,
      .right = (LONG)w,
      .bottom = (LONG)h,
  };
  pCommandList->RSSetViewports(1, &viewport);
  pCommandList->RSSetScissorRects(1, &scissor);

  pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

  render_recursive(pPanel, pCommandList, -1, -1, 0, 0);

  g_RenderingSystem.set_shader(Asset_Shader{}, pCommandList);
}

void UISystem::render_recursive(UI_Panel *pPanel,
                                ID3D12GraphicsCommandList10 *pCommandList,
                                float px, float py, float pw, float ph) {
  float x, y, w, h;

  x = pPanel->Position[0];
  y = pPanel->Position[1];

  w = pPanel->Dimensions[0];
  h = pPanel->Dimensions[1];

  // Dimension flags
  if (pPanel->Flags & UI_PANEL_FLAG_RELATIVE_SIZE_X)
    w *= pw;
  else if (pPanel->Flags & UI_PANEL_FLAG_ABSOLUTE_SIZE_X)
    w = w * 2 * m_InvScreenDimensions[0];
  else
    w = w * 2 * m_ScreenRatio * k_UIPixelScale;

  if (pPanel->Flags & UI_PANEL_FLAG_RELATIVE_SIZE_Y)
    h *= ph;
  else if (pPanel->Flags & UI_PANEL_FLAG_ABSOLUTE_SIZE_Y)
    h = h * 2 * m_InvScreenDimensions[1];
  else
    h = h * 2 * k_UIPixelScale;

  if (pPanel->Flags & UI_PANEL_FLAG_SUBTRACTIVE_SIZE_W)
    w = pw - w;

  if (pPanel->Flags & UI_PANEL_FLAG_SUBTRACTIVE_SIZE_H)
    h = ph - h;

  // Position flags
  if (pPanel->Flags & UI_PANEL_FLAG_RELATIVE_POSITION_X)
    x *= pw;
  else if (pPanel->Flags & UI_PANEL_FLAG_ABSOLUTE_POSITION_X)
    x = x * 2 * m_InvScreenDimensions[0];
  else
    x = x * 2 * m_ScreenRatio * k_UIPixelScale;

  if (pPanel->Flags & UI_PANEL_FLAG_RELATIVE_POSITION_Y)
    y *= ph;
  else if (pPanel->Flags & UI_PANEL_FLAG_ABSOLUTE_POSITION_Y)
    y = y * 2 * m_InvScreenDimensions[1];
  else
    y = y * 2 * k_UIPixelScale;

  // Get final position
  x += px + (pPanel->Anchor[0] * pw) - (pPanel->Pivot[0] * w);
  y += py + (pPanel->Anchor[1] * ph) - (pPanel->Pivot[1] * h);

  pPanel->add_render_commands(pCommandList, x, y, w, h);

  uint16_t children = pPanel->get_child_count();
  for (uint16_t i = 0; i < children; ++i) {
    render_recursive(pPanel->get_child(i), pCommandList, x, y, w, h);
  }
}

Asset_Shader UISystem::compile_transparent_quad_shader(
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