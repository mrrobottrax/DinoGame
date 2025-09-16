#include "pch.h"

#include "global.h"

#include "DGUI_Panel.h"

static DGUI_Panel s_TopPanel;
static ComPtr<ID3D12PipelineState> s_RectPipelineState;

void dgui_init(ID3D12Device9 *pDevice) {
  D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{
      .pRootSignature =,
      .VS =
          {
              .pShaderBytecode =,
              .BytecodeLength =,
          },
      .PS =
          {
              .pShaderBytecode =,
              .BytecodeLength =,
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
                  .BlendEnable = FALSE,
                  .LogicOpEnable = FALSE,
                  .SrcBlend = D3D12_BLEND_ONE,
                  .DestBlend = D3D12_BLEND_ZERO,
                  .BlendOp = D3D12_BLEND_OP_ADD,
                  .SrcBlendAlpha = D3D12_BLEND_ONE,
                  .DestBlendAlpha = D3D12_BLEND_ZERO,
                  .BlendOpAlpha = D3D12_BLEND_OP_ADD,
                  .LogicOp = D3D12_LOGIC_OP_NOOP,
                  .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL,
              }},
          },
      .SampleMask = 0,
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
              .pInputElementDescs =,
              .NumElements =,
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
      &graphicsPipelineStateDesc, IID_PPV_ARGS(&s_RectPipelineState)));
}

void dgui_stop() {
  dgui_clear_all();
  s_RectPipelineState.Reset();
}

DGUI_Panel *dgui_get_top_panel() { return &s_TopPanel; }

void dgui_clear_all() { s_TopPanel.clear_children(); }

static void render_recursive(DGUI_Panel *pPanel,
                             ID3D12GraphicsCommandList10 *pCommandList) {
  pPanel->add_render_commands(pCommandList);
  uint32_t children = pPanel->get_child_count();
  for (uint32_t i = 0; i < children; ++i) {
    render_recursive(pPanel->get_child(i), pCommandList);
  }
}

void dgui_add_render_commands(ID3D12GraphicsCommandList10 *pCommandList) {
  DGUI_Panel *pPanel = dgui_get_top_panel();
  render_recursive(pPanel, pCommandList);
}