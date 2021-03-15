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

PS_OUTPUT PS_main(VS_OUT input)
{
	// Sample from texture (DEBUG ONLY, not to be used when measuring)
	//float4 albedo   = textures[cbPerObject.info.textureAlbedo].Sample(Anisotropic16_Wrap, input.uv);
	float4 albedo = float4(0.5f, 0.5f, 0.5f, 1.0f);

	float3 finalColor = float3(0.0f, 0.0f, 0.0f);

	// Init random floats
	uint frameSeed = cbPerFrame.frameCounter + 200000;

	float invUV = 1 - input.uv.y;
	uint seed = initRand(frameSeed * input.uv.x, frameSeed * invUV);
	//uint seed = initRand(frameSeed * input.uv.x, frameSeed * input.uv.y);

	// PointLight Test
	LightHeader lHeader = rawBufferLights.Load<LightHeader>(0);
	for (int i = 0; i < lHeader.numLights; i++)
	{
		PointLight pl = rawBufferLights.Load<PointLight>(sizeof(LightHeader) + i * sizeof(PointLight));

		finalColor += CalcPointLight(pl, input.worldPos, albedo, input.norm, input.uv, seed);
	}

	float4 ambient = albedo * 0.1f;
	finalColor += ambient.rgb;
	PS_OUTPUT output;
	output.sceneColor = float4(finalColor.rgb, 1.0f);

	// DEBUG DEPTH TEXTURE (Can only see things when an object is really really close to the nearPlane, else everything is just red)
	// float depth = textures[2].Sample(Anisotropic16_Wrap, input.uv);
	// output.sceneColor = float4(depth, 0.0f, 0.0f, 1.0f);
	return output;
}
