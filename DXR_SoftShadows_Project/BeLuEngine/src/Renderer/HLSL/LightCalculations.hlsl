#include "../../Headers/GPU_Structs.h"
#include "PBRMath.hlsl"

Texture2D textures[]   : register (t0);

SamplerState Anisotropic2_Wrap	: register (s0);
SamplerState Anisotropic4_Wrap	: register (s1);
SamplerState Anisotropic8_Wrap	: register (s2);
SamplerState Anisotropic16_Wrap	: register (s3);
SamplerState samplerTypeBorder	: register (s4);

ConstantBuffer<CB_PER_SCENE_STRUCT>  cbPerScene  : register(b5, space3);

float3 CalcPointLight(
	in PointLight pointLight,
	in float4 fragPos,
	in float3 albedo,
	in float3 normal)
{
	// Basic Diffuse lightning
	float3 pointLightContribution = float3(0.0f, 0.0f, 0.0f);
	float3 lightDir = normalize(pointLight.position - fragPos.xyz);
	float NdotL = max(dot(normal, lightDir), 0.0f);

	return pointLight.baseLight.color * NdotL * albedo;
}
