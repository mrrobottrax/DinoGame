#include "UI_Quad.hlsli"

[RootSignature(ColorRootSignature)]
float4 main() : SV_TARGET
{
	return bgColor;
}