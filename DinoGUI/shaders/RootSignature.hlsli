#define DefaultRootSignature \
    "RootFlags(0), " \
    "RootConstants(num32BitConstants=20, b0)"

cbuffer RectConstantBuffer : register(b0)
{
	matrix mvpMatrix;
	float4 bgColor;
};
