#include "RootSignature.hlsli"
#include "Texture.hlsli"

Texture2D mainTex : register(t0);
SamplerState mainSampler : register(s0);

[RootSignature(TextureRootSignature)]
float4 main(VertexOut vin) : SV_TARGET
{
	//return mainTex.Sample(mainSampler, vin.uv);
	return float4(vin.uv, 0, 1);
}