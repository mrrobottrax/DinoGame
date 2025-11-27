#define _RootSignature														\
    "RootFlags(0), "														\
    "SRV(t0, flags=DATA_VOLATILE, visibility=SHADER_VISIBILITY_PIXEL)"                                                               

struct PSInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
};