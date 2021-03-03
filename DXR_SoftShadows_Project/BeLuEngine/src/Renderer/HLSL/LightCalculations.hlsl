#include "../../Headers/GPU_Structs.h"
#include "PBRMath.hlsl"

Texture2D textures[]   : register (t0);

SamplerState Anisotropic2_Wrap	: register (s0);
SamplerState Anisotropic4_Wrap	: register (s1);
SamplerState Anisotropic8_Wrap	: register (s2);
SamplerState Anisotropic16_Wrap	: register (s3);
SamplerState samplerTypeBorder	: register (s4);

ConstantBuffer<CB_PER_SCENE_STRUCT>  cbPerScene  : register(b5, space3);

// Raytracing acceleration structure, accessed as a SRV
RaytracingAccelerationStructure SceneBVH : register(t0, space4);

float RT_ShadowFactor(float3 worldPos, float3 lightDir)
{
	RayQuery<RAY_FLAG_CULL_NON_OPAQUE | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> q;
	
	uint rayFlags = 0;
	uint instanceMask = 0xff;
	
	float shadowFactor = 1.0f;
	
	RayDesc ray = (RayDesc)0;
	ray.TMin = 0.01f;
	ray.TMax = length(lightDir);
	
	ray.Direction = normalize(lightDir);
	ray.Origin = worldPos;

	q.TraceRayInline(
		SceneBVH,
		rayFlags,
		instanceMask,
		ray
	);

	q.Proceed();

	if (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
	{
		shadowFactor = 0.0f;
	}

	return shadowFactor;
}

float3 CalcPointLight(
	in PointLight pointLight,
	in float4 worldPos,
	in float3 albedo,
	in float3 normal)
{
	// Basic Diffuse lightning
	float3 lightDir = pointLight.position - worldPos.xyz;
	float NdotL = max(dot(normal, normalize(lightDir)), 0.0f);

	return albedo * pointLight.baseLight.color * NdotL * RT_ShadowFactor(worldPos.xyz, lightDir);
}
