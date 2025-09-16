#include "RootSignature.hlsli"

static const float3 k_Vertices[] =
{
	float3(1, 0, 0.5),
	float3(1, 1, 0.5),
	float3(0, 0, 0.5),
	float3(0, 1, 0.5),
};

[RootSignature(DefaultRootSignature)]
float4 main(uint index : SV_VertexID) : SV_POSITION
{
	return float4(k_Vertices[index], 1);
}