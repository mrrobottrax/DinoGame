#include "RootSignature.hlsli"

cbuffer RectConstantBuffer : register(b0)
{
    matrix mvpMatrix;
};

static const float3 k_Vertices[] =
{
	float3(1, 0, 0),
	float3(1, 1, 0),
	float3(0, 0, 0),
	float3(0, 1, 0),
};

[RootSignature(DefaultRootSignature)]
float4 main(uint index : SV_VertexID) : SV_POSITION
{
	return mul(mvpMatrix, float4(k_Vertices[index], 1));
}