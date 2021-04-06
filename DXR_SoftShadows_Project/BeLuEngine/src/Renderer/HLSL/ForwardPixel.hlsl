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

RWTexture2D<float4> light_uav[] : register(u0, space1);

SamplerState MIN_MAG_MIP_LINEAR__WRAP : register(s5);

ConstantBuffer<CB_PER_OBJECT_STRUCT> cbPerObject	  : register(b1, space3);
ConstantBuffer<DXR_CAMERA>			 cbCameraMatrices : register(b6, space3);
ByteAddressBuffer rawBufferLights : register(t0, space3);

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
    // pixel index
    float2 d = input.pos.xy - float2(0.5f, 0.5f);
    float2 uv = d / screenSize;

    float depth = textures[cbPerScene.depthBufferIndex].SampleLevel(MIN_MAG_MIP_LINEAR__WRAP, uv, 0).r;
    float3 worldPos = WorldPosFromDepth(depth, uv);

    // Init random floats
    uint frameSeed = cbPerFrame.frameCounter + 200000;
    uint seed = initRand(frameSeed * uv.x, frameSeed * uv.y);

    // PointLight Test
    LightHeader lHeader = rawBufferLights.Load<LightHeader>(0);
    for (int i = 0; i < lHeader.numLights; i++)
    {
        PointLight pl = rawBufferLights.Load<PointLight>(sizeof(LightHeader) + i * sizeof(PointLight));

        float3 lightDir = normalize(pl.position.xyz - worldPos.xyz);
        float shadowFactor = RT_ShadowFactorSoft(worldPos.xyz, pl.position.xyz, uv, lightDir, seed);
        shadowFactor = min(shadowFactor, 1);
        light_uav[i * 2 + 1][d] = shadowFactor;
    }

    return;
}