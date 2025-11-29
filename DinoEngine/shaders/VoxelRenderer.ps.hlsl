#include "VoxelRenderer.hlsli"

Texture3D<float4> tex : register(t0);

[RootSignature(_RootSignature)]
float4 main(PSInput input) : SV_TARGET
{
	float2 offset = FovMult * (input.uv * 2 - 1.f);
	
	const float3 Direction = normalize(BaseDirection.xyz + float3(offset.xy, 0));
	
	uint3 gridSize;
	//tex.GetDimensions(gridSize.x, gridSize.y, gridSize.z);
	gridSize = uint3(16, 16, 16);
	
	// check if ray collides with grid
	float3 mask = Direction < 0;
	float3 plane = lerp(float3(0, 0, 0), gridSize, mask);
		
	float3 t = (plane - BaseOrigin) / Direction;
	t = t * (1 - float3(isnan(t)));
		
	float maxT = max(t.x, max(t.y, max(t.z, 0)));
	bool3 hitPlane = t == maxT;
		
	float3 hitPoint = BaseOrigin + Direction * maxT;
		
	bool3 isInside = hitPlane | (hitPoint >= 0 & hitPoint <= gridSize);
		
	bool collides = all(isInside);
	
	if (!collides)
	{
		return float4(1, 1, 1, 1);
	}
	
	const float3 Origin = hitPoint;
	
	return float4(0, 1, 0, 1);
}