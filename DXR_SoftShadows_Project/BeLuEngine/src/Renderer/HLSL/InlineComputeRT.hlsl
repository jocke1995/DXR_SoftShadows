#include "../DXR_Helpers/shaders/Common.hlsl"
#include "LightCalculations.hlsl"

// Raytracing output texture, accessed as a UAV
RWTexture2D< float4 > gOutput : register(u0);

// Raytracing acceleration structure, accessed as a SRV
//RaytracingAccelerationStructure SceneBVH : register(t0, space4);

//Texture2D textures[]   : register (t0, space2);
SamplerState MIN_MAG_MIP_LINEAR__WRAP : register(s5);

ConstantBuffer<CB_PER_OBJECT_STRUCT> cbPerObject	  : register(b1, space3);
//ConstantBuffer<CB_PER_FRAME_STRUCT>  cbPerFrame		  : register(b4, space3);
//ConstantBuffer<CB_PER_SCENE_STRUCT> cbPerScene : register(b5, space3);
ConstantBuffer<DXR_CAMERA>			 cbCameraMatrices : register(b6, space3);
ByteAddressBuffer rawBufferLights : register(t0, space3);

static const int g_NumThreads = 256;

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
	// TODO: dont hardcode screensize
	float2 uv = dispatchThreadID.xy / float2(1280, 720);

	float depth = textures[cbPerScene.depthBufferIndex].SampleLevel(MIN_MAG_MIP_LINEAR__WRAP, uv, 0).r;
	float3 worldPos = WorldPosFromDepth(depth, uv);

	float4 normal = textures[cbPerScene.gBufferNormalIndex].SampleLevel(MIN_MAG_MIP_LINEAR__WRAP, uv, 0);

	float3 materialColor = float3(0.5f, 0.5f, 0.5f);
	float3 finalColor = float3(0.0f, 0.0f, 0.0f);

	// Init random floats
	uint frameSeed = cbPerFrame.frameCounter + 200000;
	uint seed = initRand(frameSeed * uv.x, frameSeed * (1 - uv.y));

	// PointLight Test
	LightHeader lHeader = rawBufferLights.Load<LightHeader>(0);
	for (int i = 0; i < lHeader.numLights; i++)
	{
		PointLight pl = rawBufferLights.Load<PointLight>(sizeof(LightHeader) + i * sizeof(PointLight));
		
		finalColor += CalcPointLight(pl, float4(worldPos.xyz, 1.0f), materialColor.rgb, normal.xyz, uv, seed);
	}

	float4 ambient = float4(materialColor.rgb, 1.0f) * 0.1f;
	finalColor += ambient.rgb;

	gOutput[dispatchThreadID.xy] = float4(finalColor.rgb, 1.0f);

	// WORLD POSITION
	//gOutput[dispatchThreadID.xy] = float4(worldPos.xyz/200, 1.0f);
	
	// NORMAL
	//gOutput[dispatchThreadID.xy] = float4(normal.rgb, 1.0f);

	// DEPTH
	// gOutput[dispatchThreadID.xy] = float4(depth, 0.0f, 0.0f, 1.0f);

	// UV
	// gOutput[dispatchThreadID.xy] = float4(uv.xy, 0.0f, 1.0f);

	// FLAT WHITE
	// gOutput[dispatchThreadID.xy] = float4(1.0f, 1.0f, 1.0f, 1.0f);
}