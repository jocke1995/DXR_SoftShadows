#include "../../Headers/GPU_Structs.h"

Texture2D<float4> textures[]   : register (t0);
RWTexture2D<float4> textureToBlur[] : register(u0);

ConstantBuffer<DescriptorHeapIndices> dhIndices : register(b2, space3);

static const int g_BlurRadius = 4;
static const int g_NumThreads = 256;
groupshared float4 g_SharedMem[g_NumThreads + 2 * g_BlurRadius];

[numthreads(g_NumThreads, 1, 1)]
void CS_main(uint3 dispatchThreadID : SV_DispatchThreadID, int3 groupThreadID : SV_GroupThreadID)
{
	//unsigned int readIndex = dhIndices.index0;
	//unsigned int writeIndex = dhIndices.index1;

	textureToBlur[0][dispatchThreadID.xy] = float4(1.0f, 0.0f, 0.0f, 1.0f);
}