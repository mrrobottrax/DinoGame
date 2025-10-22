#define DefaultRootSignature \
    "RootFlags(0), " \
    "RootConstants(num32BitConstants=20, b0)"

#define TextureRootSignature \
    "RootFlags(0), " \
    "RootConstants(num32BitConstants=20, b0), " \
    "DescriptorTable(SRV(t0, flags=DATA_STATIC)), " \
    "StaticSampler(s0, " \
        "addressU = TEXTURE_ADDRESS_BORDER, " \
        "addressV = TEXTURE_ADDRESS_BORDER, " \
        "borderColor = STATIC_BORDER_COLOR_TRANSPARENT_BLACK, " \
        "filter = FILTER_MIN_MAG_MIP_LINEAR )"

cbuffer RectConstantBuffer : register(b0)
{
	matrix mvpMatrix;
	float4 bgColor;
};
