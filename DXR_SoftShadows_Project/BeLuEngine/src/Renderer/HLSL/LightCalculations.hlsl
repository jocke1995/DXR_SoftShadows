#include "../../Headers/GPU_Structs.h"
#include "PBRMath.hlsl"
#include "../DXR_Helpers/shaders/hlslhelpers.hlsl"

Texture2D textures[]   : register (t0);

SamplerState Anisotropic2_Wrap	: register (s0);
SamplerState Anisotropic4_Wrap	: register (s1);
SamplerState Anisotropic8_Wrap	: register (s2);
SamplerState Anisotropic16_Wrap	: register (s3);
SamplerState samplerTypeBorder	: register (s4);

ConstantBuffer<CB_PER_FRAME_STRUCT>  cbPerFrame  : register(b4, space3);
ConstantBuffer<CB_PER_SCENE_STRUCT>  cbPerScene  : register(b5, space3);

// Raytracing acceleration structure, accessed as a SRV
RaytracingAccelerationStructure SceneBVH : register(t0, space4);

float RT_ShadowFactor(float3 worldPos, float tMin, float tMax, float3 rayDir)
{
	RayQuery<RAY_FLAG_SKIP_CLOSEST_HIT_SHADER | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> q;
	
	uint rayFlags = 0;
	uint instanceMask = 0xff;
	
	float shadowFactor = 1.0f;
	
	RayDesc ray = (RayDesc)0;
	ray.TMin = tMin;
	ray.TMax = tMax;
	
	ray.Direction = normalize(rayDir);
	ray.Origin = float4(worldPos.xyz, 1.0f);

	q.TraceRayInline(
		SceneBVH,
		rayFlags,
		instanceMask,
		ray
	);

	q.Proceed();

	if (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
	{
		shadowFactor = 0.1f;
	}

	return shadowFactor;
}

float RT_ShadowFactorSoft(float3 worldPos, float3 lightPos, float2 uv, float3 lightDir, inout uint seed)
{
	// Maybe have this attribute inside pointlight?
	float lightRadius = 1.0; // To low radius => coneAngle not accurate enough

	float3 perpL = normalize(cross(lightDir, float3(0.0f, 1.0f, 0.0f)));
	// Handle case where L = up -> perpL should then be (1,0,0)
	if (all(perpL == 0.0f))
	{
		perpL.x = 1.0;
	}

	//// Use perpL to get a vector from worldPosition to the edge of the light sphere
	float3 toLightEdge = normalize((lightPos + perpL * lightRadius) - worldPos);
	// Angle between L and toLightEdge. Used as the cone angle when sampling shadow rays
	float coneAngle = acos(dot(lightDir, toLightEdge)) * 2;

	float sumFactor = 0;

	for (int j = 0; j < cbPerScene.spp; j++)
	{
		float factor = 0;
		float3 randDir = getConeSample(seed, lightDir, coneAngle);

		factor = RT_ShadowFactor(worldPos, 1.0f, distance(lightPos, worldPos), randDir);

		sumFactor += factor;
	}

	sumFactor /= cbPerScene.spp;

	return sumFactor;
}

float3 CalcPointLight(
	in PointLight pointLight,
	in float4 worldPos,
	in float3 albedo,
	in float3 normal,
	in float2 uv,
	inout uint seed)
{
	// Basic Diffuse lightning
	float3 lightDir = normalize(pointLight.position - worldPos.xyz);
	float NdotL = max(dot(normal, lightDir), 0.0f);

	return albedo * pointLight.baseLight.color * NdotL * RT_ShadowFactorSoft(worldPos.xyz, pointLight.position, uv, lightDir, seed);
	//return albedo * pointLight.baseLight.color * NdotL * RT_ShadowFactor(worldPos.xyz, 1.0f, distance(pointLight.position, worldPos.xyz), lightDir);
	//return albedo * pointLight.baseLight.color;
	//return albedo * NdotL;
}
