#include "../DXR_Helpers/shaders/Common.hlsl"
#include "../DXR_Helpers/shaders/hlslhelpers.hlsl"

// Raytracing output texture, accessed as a UAV
RWTexture2D< float4 > gOutput : register(u0);

// Raytracing acceleration structure, accessed as a SRV
RaytracingAccelerationStructure SceneBVH : register(t0, space4);

Texture2D textures[]   : register (t0, space2);
SamplerState MIN_MAG_MIP_LINEAR__WRAP : register(s5);

ConstantBuffer<CB_PER_SCENE_STRUCT> cbPerScene : register(b5, space3);
ByteAddressBuffer rawBufferLights : register(t0, space3);

static const int g_NumThreads = 256;

[numthreads(g_NumThreads, 1, 1)]
void CS_main(uint3 dispatchThreadID : SV_DispatchThreadID, int3 groupThreadID : SV_GroupThreadID)
{
	// TODO: dont hardcode screensize
	//float2 uv = dispatchThreadID.xy / float2(1280, 720);

	//unsigned int readIndex = dhIndices.index0;
	//unsigned int writeIndex = dhIndices.index1;

	gOutput[dispatchThreadID.xy] = float4(1.0f, 0.0f, 0.0f, 1.0f);
}