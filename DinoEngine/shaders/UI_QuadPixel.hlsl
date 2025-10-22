#include "UI_Quad.hlsli"

[RootSignature(DefaultRootSignature)]
float4 main() : SV_TARGET
{
	return bgColor;
}