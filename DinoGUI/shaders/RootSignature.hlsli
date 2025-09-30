#define DefaultRootSignature \
    "RootFlags(0), " \
    "RootConstants(num32BitConstants=20, b0)"

#define TextureRootSignature \
    "RootFlags(0), " \
    "RootConstants(num32BitConstants=20, b0), " \
    "DescriptorTable(SRV(t0, flags=DATA_STATIC)), " \
    "Sampler(s0, addressU = BORDER, addressV = BORDER, borderColor = TRANSPARENT_BLACK, filter = MIN_MAG_MIP_LINEAR)"

cbuffer RectConstantBuffer : register(b0)
{
	matrix mvpMatrix;
	float4 bgColor;
};
