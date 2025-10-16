#define _RootSignature \
    "RootFlags(0), " \
    "RootConstants(num32BitConstants=2, b0), " \
    "UAV(u0, flags=DATA_VOLATILE)"

cbuffer RectConstantBuffer : register(b0)
{
	uint2 max;
};

RWTexture2D<float4> _texture : register(u0);

[numthreads(16, 16, 1)]
[RootSignature(_RootSignature)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    
}