#include "RootSignature.hlsli"
#include "Texture.hlsli"

static const float3 k_Vertices[] =
{
	float3(1, 0, 0),
	float3(1, 1, 0),
	float3(0, 0, 0),
	float3(0, 1, 0),
};

static const float2 k_UVs[] =
{
	float2(1, 0),
	float2(1, 1),
	float2(0, 0),
	float2(0, 1),
};

[RootSignature(TextureRootSignature)]
VertexOut main(uint index : SV_VertexID)
{
	VertexOut vout;
	vout.position = mul(mvpMatrix, float4(k_Vertices[index], 1));
	vout.uv = k_UVs[index];
	return vout;
}