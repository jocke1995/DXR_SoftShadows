#include "../../Headers/GPU_Structs.h"

struct VS_OUT
{
	float4 pos  : SV_Position;
	float2 uv   : UV;
};

Texture2D<float4> textures[]   : register (t0, space10);
RWTexture2D<float4> shadowBuffer[] : register(u0, space1);

SamplerState point_Wrap	: register (s5);

void PS_main(VS_OUT input)
{

	float sum = 0;
	for (int i = 0; i < NUM_TEMPORAL_BUFFERS + 1; i++)
	{
		sum = sum + textures[i * 2 * MAX_POINT_LIGHTS].Sample(point_Wrap, input.uv);
	}
	sum = sum / (NUM_TEMPORAL_BUFFERS + 1);

	shadowBuffer[1][input.pos.xy] = min(sum, 1.0);

	return;
}
