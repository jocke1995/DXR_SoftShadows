#include "LightCalculations.hlsl"

struct VS_OUT
{
	float4 pos      : SV_Position;
	float4 worldPos : WPos;
	float2 uv       : UV;
	float3 norm		: NORM;
};

ByteAddressBuffer rawBufferLights: register(t0, space3);

ConstantBuffer<CB_PER_OBJECT_STRUCT> cbPerObject : register(b1, space3);
RWTexture2D<float4> light_uav[] : register(u0, space1);

[earlydepthstencil]
void PS_main(VS_OUT input)
{
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

		float3 lightDir = normalize(pl.position.xyz - input.worldPos.xyz);
		float shadowFactor = RT_ShadowFactorSoft(input.worldPos.xyz, pl.position.xyz, input.uv, lightDir, seed);
		shadowFactor = min(shadowFactor, 1);

		light_uav[i * 2 + 1][input.pos.xy] = shadowFactor;
	}

	return;
}
