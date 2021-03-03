#include "LightCalculations.hlsl"

struct VS_OUT
{
	float4 pos      : SV_Position;
	float4 worldPos : WPos;
	float2 uv       : UV;
	float3 norm		: NORM;
};

struct PS_OUTPUT
{
	float4 sceneColor: SV_TARGET0;
};

ByteAddressBuffer rawBufferLights: register(t0, space3);

ConstantBuffer<CB_PER_OBJECT_STRUCT> cbPerObject : register(b1, space3);
ConstantBuffer<CB_PER_FRAME_STRUCT>  cbPerFrame  : register(b4, space3);

// Raytracing acceleration structure, accessed as a SRV
RaytracingAccelerationStructure SceneBVH : register(t0, space4);


PS_OUTPUT PS_main(VS_OUT input)
{
	// Sample from texture
	float4 albedo   = textures[cbPerObject.info.textureAlbedo	].Sample(Anisotropic16_Wrap, input.uv);

	//normal = (2.0f * normal) - 1.0f;
	//normal = float4(normalize(mul(normal.xyz, input.tbn)), 1.0f);

	float3 finalColor = float3(1.0f, 0.0f, 0.0f);

	// PointLight Test
	LightHeader lHeader = rawBufferLights.Load<LightHeader>(0);
	for (int i = 0; i < lHeader.numLights; i++)
	{
		PointLight pl = rawBufferLights.Load<PointLight>(sizeof(LightHeader) + i * sizeof(PointLight));

		finalColor += CalcPointLight(pl, input.worldPos, albedo, input.norm);
	}

	PS_OUTPUT output;
	output.sceneColor = float4(finalColor.rgb, 1.0f);
	return output;
}
