#include "../../Headers/GPU_Structs.h"

struct VS_OUT
{
	float4 pos      : SV_Position;
	float4 worldPos : WPos;
	float2 uv       : UV;
	float3 norm		: NORM;
};

struct vertex
{
	float3 pos;
	float2 uv;
	float3 norm;
	float3 tang;
};

ConstantBuffer<CB_PER_OBJECT_STRUCT> cbPerObject : register(b1, space3);

StructuredBuffer<vertex> meshes[] : register(t0);

VS_OUT VS_main(uint vID : SV_VertexID)
{
	VS_OUT output = (VS_OUT)0;

	vertex v = meshes[cbPerObject.info.vertexDataIndex][vID];
	float4 vertexPosition = float4(v.pos.xyz, 1.0f);

	output.pos = mul(vertexPosition, cbPerObject.WVP);
	output.worldPos = mul(vertexPosition, cbPerObject.worldMatrix);

	output.uv = float2(v.uv.x, v.uv.y);

	output.norm = normalize(mul(float4(v.norm, 0.0f), cbPerObject.worldMatrix)).xyz;


	return output;
}