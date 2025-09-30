#pragma once

DGUI_API void
dgui_add_render_commands(ID3D12GraphicsCommandList10 *pCommandList,
                         unsigned int w, unsigned int h);

struct ShaderData {
  ID3D12PipelineState *pPipelineState;
  ID3D12RootSignature *pRootSignature;
};

DGUI_API void dgui_compile_shader(ID3D12Device9 *pDevice,
                                  ShaderData *pShaderData,
                                  const wchar_t *vertexPath,
                                  const wchar_t *pixelPath);

DGUI_API void dgui_release_shader(ShaderData *pShaderData);

DGUI_API void dgui_set_shader(ShaderData *pShaderData,
                              ID3D12GraphicsCommandList10 *pCommandList);

DGUI_API extern ShaderData g_RectShader;