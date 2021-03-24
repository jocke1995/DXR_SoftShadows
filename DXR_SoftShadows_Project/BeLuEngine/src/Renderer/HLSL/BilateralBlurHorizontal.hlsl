#include "../../Headers/GPU_Structs.h"
#include "../DXR_Helpers/shaders/hlslhelpers.hlsl"

Texture2D<float4> textures[]   : register (t0);
RWTexture2D<float4> textureToBlur[] : register(u0, space1);

ConstantBuffer<DescriptorHeapIndices> dhIndices : register(b9, space3);
ConstantBuffer<CB_PER_SCENE_STRUCT> cbPerScene : register(b5, space3);
ConstantBuffer<DXR_CAMERA>           cbCameraMatrices : register(b6, space3);

SamplerState MIN_MAG_MIP_LINEAR__WRAP : register(s5);
SamplerState MIN_MAG_MIP_POINT__WRAP : register(s6);

static const int g_BlurRadius = 4;
static const int g_NumThreads = 256;

groupshared float4 g_SharedMem[g_NumThreads + 2 * g_BlurRadius];

// Calculate world pos from DepthBuffer
float3 WorldPosFromDepth(float depth, float2 TexCoord)
{
	TexCoord.y = 1.0 - TexCoord.y;
	float4 clipSpacePosition = float4(TexCoord * 2.0 - 1.0, depth, 1.0);
	float4 viewSpacePosition = mul(cbCameraMatrices.projectionI, clipSpacePosition);

	// Perspective division
	viewSpacePosition /= viewSpacePosition.w;

	float4 worldSpacePosition = mul(cbCameraMatrices.viewI, viewSpacePosition);

	return worldSpacePosition.xyz;
}

[numthreads(g_NumThreads, 1, 1)]
void CS_main(uint3 dispatchThreadID : SV_DispatchThreadID, int3 groupThreadID : SV_GroupThreadID)
{
	
	float2 uv = dispatchThreadID.xy / screenSize;
	
	/* Sample depth and normal from textures */
	float depth = linearizeDepth(textures[cbPerScene.depthBufferIndex].SampleLevel(MIN_MAG_MIP_POINT__WRAP, uv, 0).r);
	float3 normal = normalize(textures[cbPerScene.gBufferNormalIndex].SampleLevel(MIN_MAG_MIP_POINT__WRAP, uv, 0).rgb);
	float depthWorld = WorldPosFromDepth(depth, uv);
	
	/* DescriptorHeap indices */
	unsigned int readIndex = dhIndices.index0;
	unsigned int writeIndex = dhIndices.index1;

	float weights[5] = { 0.227027f, 0.1945946f, 0.1216216f, 0.054054f, 0.016216f };

	/* -------------------- Clamp out of bound samples -------------------- */
	// left side
	if (groupThreadID.x < g_BlurRadius)
	{
		int x = max(dispatchThreadID.x - g_BlurRadius, 0);
		g_SharedMem[groupThreadID.x] = textures[readIndex][int2(x, dispatchThreadID.y)];
	}

	uint mipLevel;
	uint width;
	uint height;
	uint numLevels;
	textures[readIndex].GetDimensions(mipLevel, width, height, numLevels);
	
	// right side
	if (groupThreadID.x >= g_NumThreads - g_BlurRadius)
	{
		int x = min(dispatchThreadID.x + g_BlurRadius, width - 1);
		g_SharedMem[groupThreadID.x + 2 * g_BlurRadius] = textures[readIndex][int2(x, dispatchThreadID.y)];
	}
	/* -------------------- Clamp out of bound samples -------------------- */

	// Fill the middle parts of the sharedMemory
	g_SharedMem[groupThreadID.x + g_BlurRadius] = textures[readIndex][min(dispatchThreadID.xy, float2(width, height) - 1)];

	// Wait for shared memory to be populated before reading from it
	GroupMemoryBarrierWithGroupSync();
	
	// Blur
	// Current fragments contribution
	float4 blurColor = weights[0] * g_SharedMem[groupThreadID.x + g_BlurRadius];
	float totalWeight = weights[0];

	// Adjacent fragment contributions
	for (int i = 1; i <= g_BlurRadius; i++)
	{
		// Left side
		float2 uvLeft = (dispatchThreadID.xy - float2(i, 0)) / screenSize;
		float depthLeft = linearizeDepth(textures[cbPerScene.depthBufferIndex].SampleLevel(MIN_MAG_MIP_POINT__WRAP, uvLeft, 0).r);
		float depthLeftWorld = WorldPosFromDepth(depthLeft, uvLeft);
		float3 normalLeft = normalize(textures[cbPerScene.gBufferNormalIndex].SampleLevel(MIN_MAG_MIP_POINT__WRAP, uvLeft, 0).rgb);

		int left = groupThreadID.x + g_BlurRadius - i;
		if (abs(depthLeftWorld - depthWorld) <= 0.2f)	// Skip pixels if the neighbor values differ to much
		{
			blurColor += weights[i] * g_SharedMem[left];
			totalWeight += weights[i];
		}

		// Right side
		float2 uvRight = (dispatchThreadID.xy + float2(i, 0)) / screenSize;
		float depthRight = linearizeDepth(textures[cbPerScene.depthBufferIndex].SampleLevel(MIN_MAG_MIP_POINT__WRAP, uvRight, 0).r);
		float depthRightWorld = WorldPosFromDepth(depthRight, uvRight);
		float3 normalRight = normalize(textures[cbPerScene.gBufferNormalIndex].SampleLevel(MIN_MAG_MIP_POINT__WRAP, uvRight, 0).rgb);

		int right = groupThreadID.x + g_BlurRadius + i;
		if (abs(depthRightWorld - depthWorld) <= 0.2f)	// Skip pixels if the neighbor values differ to much
		{
			blurColor += weights[i] * g_SharedMem[right];
			totalWeight += weights[i];
		}
	}

	textureToBlur[writeIndex][dispatchThreadID.xy] = blurColor / totalWeight;
}
