#include "FullScreenQuad.hlsli"

Texture2D RenderTexture : register(t0);
SamplerState PointSampler : register(s0);

[RootSignature(_RootSignature)]
float4 main() : SV_TARGET
{
	return float4(0, 1, 0, 1);
}