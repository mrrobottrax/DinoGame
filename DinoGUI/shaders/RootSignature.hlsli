#define DefaultRootSignature \
    "RootFlags(0), " \
    "RootConstants(num32BitConstants=20, b0)"

#define TextureRootSignature \
    "RootFlags(0), " \
    "RootConstants(num32BitConstants=20, b0), " \
    "SRV(t0), " \
    "StaticSampler(s0)"

cbuffer RectConstantBuffer : register(b0)
{
	matrix mvpMatrix;
	float4 bgColor;
};
