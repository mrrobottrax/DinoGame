#include "VoxelRenderer.hlsli"

Texture2D<float4> tex : register(t0);

[RootSignature(_RootSignature)]
float4 main(PSInput input) : SV_TARGET
{
	return float4(1.0f, 1.0f, 1.0f, 1.0f);
}