#define _RootSignature                                                                      \
    "RootFlags(0), "                                                                        \
    "DescriptorTable(SRV(t0, flags=DATA_VOLATILE), visibility=SHADER_VISIBILITY_PIXEL), "   \
    "StaticSampler(s0, "                                                                    \
    "filter = FILTER_MIN_MAG_MIP_POINT, "                                                   \
    "addressU = TEXTURE_ADDRESS_CLAMP, "                                                    \
    "addressV = TEXTURE_ADDRESS_CLAMP, "                                                    \
    "addressW = TEXTURE_ADDRESS_CLAMP)"

struct PSInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
};