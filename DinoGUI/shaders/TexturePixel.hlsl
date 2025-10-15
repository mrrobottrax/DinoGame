#include "RootSignature.hlsli"
#include "Texture.hlsli"

Texture2D mainTex : register(t0);
SamplerState mainSampler : register(s0);

// Linear to SRGB
//float4 fromLinear(float4 linearRGB)
//{
//    bool3 cutoff = linearRGB.rgb < 0.0031308;
//    float3 higher = 1.055 * pow(linearRGB.rgb, 1.0 / 2.4) - 0.055;
//    float3 lower = linearRGB.rgb * 12.92;

//    return float4(lerp(higher, lower, cutoff), linearRGB.a);
//}

[RootSignature(TextureRootSignature)]
float4 main(VertexOut vin) : SV_TARGET
{
	return mainTex.Sample(mainSampler, vin.uv) * bgColor;
}