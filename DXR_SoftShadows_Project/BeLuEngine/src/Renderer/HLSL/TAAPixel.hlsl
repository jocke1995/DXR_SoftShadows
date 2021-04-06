#include "../../Headers/GPU_Structs.h"

struct VS_OUT
{
	float4 pos  : SV_Position;
	float2 uv   : UV;
};

RWTexture2D<float4> currFrame[]   : register (u0, space0);
RWTexture2D<float4> TAABuffer[] : register(u0, space1);

SamplerState point_Wrap	: register (s5);

void PS_main(VS_OUT input)
{
	float2 correctPos = input.pos.xy - float2(0.5f, 0.5f);

	float3 currColor = currFrame[0][correctPos];
	float3 oldColor = TAABuffer[0][correctPos];

	float factor = 0.05f; // CurrFrame "weight"
	float3 newColor = lerp(oldColor, currColor, factor);

	// Write newColor to TAABuffer
	float4 f = float4(newColor, 1.0);
	TAABuffer[0][correctPos] = f;
	currFrame[0][correctPos] = f;

	return;
}
