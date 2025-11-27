#include "FullScreenQuad.hlsli"

Texture2D RenderTexture : register(t0);
SamplerState PointSampler : register(s0);

float4 linear_to_srgb(float4 og)
{
	float3 lin = og.rgb;

	for (int i = 0; i < 3; i++)
	{
		if (lin[i] <= 0.0031308)
			lin[i] = 12.92 * lin[i];
		else
			lin[i] = 1.055 * pow(lin[i], 1.0 / 2.4) - 0.055;
	}

	return float4(lin, og.a);
}

[RootSignature(_RootSignature)]
float4 main(PSInput input) : SV_TARGET
{
	float4 og = RenderTexture.Sample(PointSampler, input.uv);
	return linear_to_srgb(og);
}