#include "RootSignature.hlsli"

[RootSignature(DefaultRootSignature)]
float4 main() : SV_TARGET
{
	return bgColor;
}