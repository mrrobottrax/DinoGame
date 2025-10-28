#include "UI_Img.hlsli"

Texture2D tex : register(t0);
SamplerState samp : register(s0);

[RootSignature(TextureRootSignature)]
float4 main(PSInput input) : SV_TARGET
{
	float2 uv = input.uv;
	
	return bgColor * float4(uv.x, uv.y, 0, 1);
}