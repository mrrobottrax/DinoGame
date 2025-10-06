#pragma once

DGUI_API void
DGUI_add_render_commands(ID3D12GraphicsCommandList10 *pCommandList,
                         unsigned int w, unsigned int h);

struct ShaderData {
  Microsoft::WRL::ComPtr<ID3D12PipelineState> pPipelineState;
  Microsoft::WRL::ComPtr<ID3D12RootSignature> pRootSignature;
};

/// <summary>
/// If the root signature exists in the ShaderData, use that. Otherwise, create
/// one from the shader.
/// </summary>
DGUI_API void DGUI_compile_shader(ID3D12Device9 *pDevice,
                                  ShaderData *pShaderData,
                                  const wchar_t *vertexPath,
                                  const wchar_t *pixelPath);

DGUI_API void DGUI_release_shader(ShaderData *pShaderData);

DGUI_API void DGUI_set_shader(ShaderData *pShaderData,
                              ID3D12GraphicsCommandList10 *pCommandList);

DGUI_API extern ShaderData g_RectShader;
DGUI_API extern ShaderData g_TextureShader;