#include "LightCalculations.hlsl" 

struct VS_OUT
{
    float4 pos  : SV_Position;
    float2 uv   : UV;
    float3 normal : NORMAL;
};

struct RootConstant4
{
    unsigned int depthBufferIndice;
    unsigned int gBufferIndice;
    unsigned int c;
    unsigned int d;
};

RWTexture2D< float4 > gOutput : register(u0);

Texture2D<float4> shadowBuffers[]   : register (t0, space10);
ByteAddressBuffer rawBufferLights: register(t0, space3);

ConstantBuffer<DXR_CAMERA>  cbCameraMatrices  : register(b6, space3);
ConstantBuffer<RootConstant4> CBindices : register(b9, space3);

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
    float2 d = (input.pos.xy - float2(0.5f, 0.5f)) / screenSize;

    float depthSample = textures[CBindices.depthBufferIndice].Sample(point_Wrap, d.xy).r;
    float3 worldPos = WorldPosFromDepth(depthSample, d.xy);
    float3 normal = textures[CBindices.gBufferIndice].Sample(point_Wrap, d.xy).rgb;

    float3 materialColor = float3(0.5, 0.5, 0.5);
    float3 ambient = materialColor * float3(0.1f, 0.1f, 0.1f);
    float3 finalColor = float3(0, 0, 0);

    LightHeader lHeader = rawBufferLights.Load<LightHeader>(0);
    for (unsigned int i = 0; i < lHeader.numLights; i++)
    {
        PointLight pl = rawBufferLights.Load<PointLight>(sizeof(LightHeader) + i * sizeof(PointLight));

        float shadowFactor = shadowBuffers[i*2].Sample(point_Wrap, d.xy).r;

        finalColor += CalcPointLight2(pl, float4(worldPos, 1), materialColor, normal, d.xy, shadowFactor);
    }

    finalColor += ambient;

    gOutput[input.pos.xy] = float4(finalColor, 1);

    return;
}