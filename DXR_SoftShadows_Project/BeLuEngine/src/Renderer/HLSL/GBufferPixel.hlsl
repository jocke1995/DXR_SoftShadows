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

//ByteAddressBuffer rawBufferLights: register(t0, space3);
//
//ConstantBuffer<CB_PER_OBJECT_STRUCT> cbPerObject : register(b1, space3);
//ConstantBuffer<CB_PER_FRAME_STRUCT>  cbPerFrame  : register(b4, space3);

PS_OUTPUT PS_main(VS_OUT input)
{
	PS_OUTPUT output;
	output.sceneColor = float4(input.norm, 1.0f);

	return output;
}
