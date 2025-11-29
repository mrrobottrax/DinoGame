#define _RootSignature														                \
    "RootFlags(0), "														                \
    "RootConstants(num32BitConstants=10, b0), "												\
    "DescriptorTable(SRV(t0, flags=DATA_VOLATILE), visibility=SHADER_VISIBILITY_PIXEL), "                                                

cbuffer ConstantData : register(b0)
{
	float3 BaseOrigin;
	float _padding0;
	float3 BaseDirection;
	float _padding1;
	float2 FovMult;
};

struct PSInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
};