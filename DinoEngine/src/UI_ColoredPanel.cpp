#include "pch.h"

#include "IUISystem.h"
#include "UI_ColoredPanel.h"

void UI_ColoredPanel::add_render_commands(
    ID3D12GraphicsCommandList10 *pCommandList, float x, float y, float w,
    float h) {
  mat4_t matrix = mat4_create(x, y, 0, w, h);

  pCommandList->SetPipelineState(g_UIRectShader.PipelineState);
  pCommandList->SetGraphicsRootSignature(g_UIRectShader.RootSignature);

  pCommandList->SetGraphicsRoot32BitConstants(0, 16, matrix.Data, 0);
  pCommandList->SetGraphicsRoot32BitConstants(0, 4, Color, 16);

  pCommandList->DrawInstanced(4, 1, 0, 0);
}