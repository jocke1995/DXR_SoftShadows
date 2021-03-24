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

[numthreads(1, g_NumThreads, 1)]
void CS_main(uint3 dispatchThreadID : SV_DispatchThreadID, int3 groupThreadID : SV_GroupThreadID)
{
	float2 uv = dispatchThreadID.xy / screenSize;
	
	/* Sample depth and normal from textures */
	float depth = linearizeDepth(textures[cbPerScene.depthBufferIndex].SampleLevel(MIN_MAG_MIP_POINT__WRAP, uv, 0).r);
	float3 normal = normalize(textures[cbPerScene.gBufferNormalIndex].SampleLevel(MIN_MAG_MIP_POINT__WRAP, uv, 0).rgb);
	float depthWorld = WorldPosFromDepth(depth, uv);


	/* DescriptorHeap indices */
	unsigned int readIndex = dhIndices.index2;
	unsigned int writeIndex = dhIndices.index3;

	float weights[5] = { 0.227027f, 0.1945946f, 0.1216216f, 0.054054f, 0.016216f };
	/* -------------------- Clamp out of bound samples -------------------- */
	// top side
	if (groupThreadID.y < g_BlurRadius)
	{
		int y = max(dispatchThreadID.y - g_BlurRadius, 0);
		g_SharedMem[groupThreadID.y] = textures[readIndex][int2(dispatchThreadID.x, y)];
	}

	uint mipLevel;
	uint width;
	uint height;
	uint numLevels;
	textures[readIndex].GetDimensions(mipLevel, width, height, numLevels);

	// bot side
	if (groupThreadID.y >= g_NumThreads - g_BlurRadius)
	{
		int y = min(dispatchThreadID.y + g_BlurRadius, height - 1);
		g_SharedMem[groupThreadID.y + 2 * g_BlurRadius] = textures[readIndex][int2(dispatchThreadID.x, y)];
	}
	/* -------------------- Clamp out of bound samples -------------------- */

	// Fill the middle parts of the sharedMemory
	g_SharedMem[groupThreadID.y + g_BlurRadius] = textures[readIndex][min(dispatchThreadID.xy, float2(width, height) - 1)];

	// Wait for shared memory to be populated before reading from it
	GroupMemoryBarrierWithGroupSync();

	// Blur
	// Current fragments contribution
	float4 blurColor = weights[0] * g_SharedMem[groupThreadID.y + g_BlurRadius];
	float totalWeight = weights[0];

	// 1 1 1 1 1
	// 1 1 1 1 1
	// 1 1 1 1 1
	// 1 1 1 1 1
	// 1 1 1 1 1

	// Adjacent fragment contributions
	for (int i = 1; i <= g_BlurRadius; i++)
	{
		// Top side
		float2 uvTop = (dispatchThreadID.xy - float2(0, i)) / screenSize;
		float depthTop = linearizeDepth(textures[cbPerScene.depthBufferIndex].SampleLevel(MIN_MAG_MIP_POINT__WRAP, uvTop, 0).r);
		float depthTopWorld = WorldPosFromDepth(depthTop, uvTop);
		float3 normalTop = normalize(textures[cbPerScene.gBufferNormalIndex].SampleLevel(MIN_MAG_MIP_POINT__WRAP, uvTop, 0).rgb);

		int top = groupThreadID.y + g_BlurRadius - i;
		if (abs(depthTopWorld - depthWorld) <= 0.2f)	// Skip pixels if the neighbor values differ to much
		{
			blurColor += weights[i] * g_SharedMem[top];
			totalWeight += weights[i];
		}

		// Bot side
		float2 uvBot = (dispatchThreadID.xy + float2(0, i)) / screenSize;
		float depthBot = linearizeDepth(textures[cbPerScene.depthBufferIndex].SampleLevel(MIN_MAG_MIP_POINT__WRAP, uvBot, 0).r);
		float depthBotWorld = WorldPosFromDepth(depthBot, uvBot);
		float3 normalBot = normalize(textures[cbPerScene.gBufferNormalIndex].SampleLevel(MIN_MAG_MIP_POINT__WRAP, uvBot, 0).rgb);

		int bot = groupThreadID.y + g_BlurRadius + i;
		if (abs(depthBotWorld - depthWorld) <= 0.2f)	// Skip pixels if the neighbor values differ to much
		{
			blurColor += weights[i] * g_SharedMem[bot];
			totalWeight += weights[i];
		}
	}

	textureToBlur[writeIndex][dispatchThreadID.xy] = blurColor / totalWeight;
}