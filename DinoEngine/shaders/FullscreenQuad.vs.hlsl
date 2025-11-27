#include "FullScreenQuad.hlsli"

static const float3 k_Vertices[] =
{
	float3(1, -1, 0),
	float3(1, 1, 0),
	float3(-1, -1, 0),
	float3(-1, 1, 0),
};

static const float2 k_UVs[] =
{
	float2(1, 1),
	float2(1, 0),
	float2(0, 1),
	float2(0, 0),
};

[RootSignature(_RootSignature)]
PSInput main(uint index : SV_VertexID)
{
	PSInput output;
	
	output.position = float4(k_Vertices[index], 1);
	output.uv = float2(k_UVs[index]);
	
	return output;
}