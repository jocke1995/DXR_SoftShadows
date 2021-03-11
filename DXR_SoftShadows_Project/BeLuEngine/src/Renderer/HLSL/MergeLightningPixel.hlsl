#include "LightCalculations.hlsl"

struct VS_OUT
{
	float4 pos  : SV_Position;
	float2 uv   : UV;
    float3 normal : NORMAL;
};

RWTexture2D< float4 > gOutput : register(u0);

Texture2D<float4> shadowBuffers[]   : register (t0, space10);
ByteAddressBuffer rawBufferLights: register(t0, space3);
ConstantBuffer<DXR_CAMERA>  cbCameraMatrices  : register(b6, space3);

SamplerState point_Wrap	: register (s5);


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


void PS_main(VS_OUT input)
{
    int i = 0;
    PointLight pl = rawBufferLights.Load<PointLight>(sizeof(LightHeader) + i * sizeof(PointLight));
    float3 lightPos = pl.position.xyz;
    float3 lightColor = pl.baseLight.color;

    float2 d = input.pos.xy / float2(1280, 720);
    float3 worldPos = WorldPosFromDepth(input.pos.w, d.xy);
    float3 normal = input.normal;

    float3 lightDir = normalize(lightPos - worldPos);

    float nDotL = max(0.0f, dot(normal, lightDir));
    
    float shadowSum = 1;
    float3 finalColor = float3(0, 0, 0);

    float3 materialColor = float3(0.5, 0.5, 0.5);
    finalColor += materialColor * lightColor * shadowSum * nDotL;

	float3 ambient = materialColor * float3(0.1f, 0.1f, 0.1f);
	finalColor = finalColor + ambient;

	gOutput[input.pos.xy] = float4(worldPos.xy, 1, 1);

	return;
}