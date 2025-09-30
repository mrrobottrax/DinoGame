#include "RootSignature.hlsli"
#include "Texture.hlsli"

[RootSignature(TextureRootSignature)]
float4 main(VertexOut vin) : SV_TARGET
{
	return float4(vin.uv, 0, 1);
}