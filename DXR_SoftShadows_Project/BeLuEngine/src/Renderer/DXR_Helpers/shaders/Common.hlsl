#include "../../../Headers/GPU_Structs.h"
ConstantBuffer<CB_PER_OBJECT_STRUCT> cbPerObject	  : register(b1, space3);
ConstantBuffer<CB_PER_FRAME_STRUCT>  cbPerFrame		  : register(b4, space3);
ConstantBuffer<DXR_CAMERA>			 cbCameraMatrices : register(b6, space3);

struct vertex
{
	float3 pos;
	float2 uv;
	float3 norm;
	float3 tang;
};

// Hit information, aka ray payload
// This sample only carries a shading color and hit distance.
// Note that the payload should be kept as small as possible,
// and that its size must be declared in the corresponding
// D3D12_RAYTRACING_SHADER_CONFIG pipeline subobjet.
struct HitInfo
{
  float4 colorAndDistance;
};

// Ray payload for the shadow rays
struct ShadowHitInfo
{
    bool isHit;
};

// Attributes output by the raytracing when hitting a surface,
// here the barycentric coordinates
struct Attributes
{
  float2 bary;
};
