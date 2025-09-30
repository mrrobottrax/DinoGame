#define DefaultRootSignature \
    "RootFlags(0), " \
    "RootConstants(num32BitConstants=20, b0)"

#define TextureRootSignature \
    "RootFlags(0), " \
    "RootConstants(num32BitConstants=20, b0), " \
    "DescriptorTable(SRV(t0, flags=DATA_STATIC)), " \
    "StaticSampler(s0)"

cbuffer RectConstantBuffer : register(b0)
{
	matrix mvpMatrix;
	float4 bgColor;
};
