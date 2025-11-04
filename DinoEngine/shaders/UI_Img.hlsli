#define TextureRootSignature \
    "RootFlags(0), " \
    "RootConstants(num32BitConstants=20, b0), " \
    "DescriptorTable(SRV(t0, flags=DATA_STATIC), visibility=SHADER_VISIBILITY_PIXEL), " \
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

struct PSInput
{
	float4 position : SV_POSITION;
    float2 uv       : TEXCOORD0;
};