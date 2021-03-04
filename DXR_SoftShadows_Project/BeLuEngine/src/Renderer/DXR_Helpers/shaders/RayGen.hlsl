#include "Common.hlsl"

// Raytracing output texture, accessed as a UAV
RWTexture2D< float4 > gOutput : register(u0);

// Raytracing acceleration structure, accessed as a SRV
RaytracingAccelerationStructure SceneBVH : register(t0, space4);

Texture2D textures[]   : register (t0, space2);
SamplerState MIN_MAG_MIP_LINEAR__WRAP : register(s5);

ConstantBuffer<CB_PER_SCENE_STRUCT> cbPerScene : register(b5, space3);
ByteAddressBuffer rawBufferLights : register(t0, space3);

// Calculate world pos from DepthBuffer
float3 WorldPosFromDepth(float depth, float2 TexCoord)
{
	float z = depth * 2.0 - 1.0;

	float4 clipSpacePosition = float4(TexCoord * 2.0 - 1.0, z, 1.0);
	float4 viewSpacePosition = mul(cbCameraMatrices.projectionI, clipSpacePosition);

	// Perspective division
	viewSpacePosition /= viewSpacePosition.w;

	float4 worldSpacePosition = mul(cbCameraMatrices.viewI, viewSpacePosition);

	return worldSpacePosition.xyz;
}

[shader("raygeneration")] 
void RayGen() {
	// Get the location within the dispatched 2D grid of work items
	// (often maps to pixels, so this could represent a pixel coordinate).
	uint2 launchIndex = DispatchRaysIndex();
	
	float2 dims = float2(DispatchRaysDimensions().xy);

    // UV:s (0->1)
	float2 uv = ((launchIndex.xy + 0.5f) / dims.xy);

	float depth = textures[2].SampleLevel(MIN_MAG_MIP_LINEAR__WRAP, uv, 0).r;

	float3 worldPos = WorldPosFromDepth(depth, uv);


    float3 materialColor = float3(0.5f, 0.5f, 0.5f);
    float3 finalColor = float3(0.0f, 0.0f, 0.0f);

    // PointLight Test
    LightHeader lHeader = rawBufferLights.Load<LightHeader>(0);
    for (int i = 0; i < lHeader.numLights; i++)
    {
        PointLight pl = rawBufferLights.Load<PointLight>(sizeof(LightHeader) + i * sizeof(PointLight));
        float3 lightPos = pl.position.xyz;
        float3 lightColor = pl.baseLight.color;
        // Find the world - space hit position
        float3 lightDir = normalize(lightPos - worldPos);

        // Fire a shadow ray. The direction is hard-coded here, but can be fetched
        // from a constant-buffer
        RayDesc ray;
        ray.Origin = float4(worldPos.xyz, 1.0f);
        ray.Direction = lightDir;
        ray.TMin = 0.00001;
        ray.TMax = distance(lightPos, worldPos);

        // Initialize the ray payload
        ShadowHitInfo shadowPayload;
        shadowPayload.isHit = false;

        // Trace the ray
        TraceRay(
            // Acceleration structure
            SceneBVH,
            // Flags can be used to specify the behavior upon hitting a surface
            //RAY_FLAG_SKIP_CLOSEST_HIT_SHADER | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH,
            RAY_FLAG_NONE,
            // Instance inclusion mask, which can be used to mask out some geometry to
            // this ray by and-ing the mask with a geometry mask. The 0xFF flag then
            // indicates no geometry will be masked
            0xFF,
            // Depending on the type of ray, a given object can have several hit
            // groups attached (ie. what to do when hitting to compute regular
            // shading, and what to do when hitting to compute shadows). Those hit
            // groups are specified sequentially in the SBT, so the value below
            // indicates which offset (on 4 bits) to apply to the hit groups for this
            // ray. In this sample we only have one hit group per object, hence an
            // offset of 0.
            0,
            // The offsets in the SBT can be computed from the object ID, its instance
            // ID, but also simply by the order the objects have been pushed in the
            // acceleration structure. This allows the application to group shaders in
            // the SBT in the same order as they are added in the AS, in which case
            // the value below represents the stride (4 bits representing the number
            // of hit groups) between two consecutive objects.
            0,
            // Index of the miss shader to use in case several consecutive miss
            // shaders are present in the SBT. This allows to change the behavior of
            // the program when no geometry have been hit, for example one to return a
            // sky color for regular rendering, and another returning a full
            // visibility value for shadow rays. This sample has only one miss shader,
            // hence an index 0
            0,
            // Ray information to trace
            ray,
            // Payload associated to the ray, which will be used to communicate
            // between the hit/miss shaders and the raygen
            shadowPayload);

        float factor = shadowPayload.isHit ? 0.0 : 1.0;

        float nDotL = 1.0f;// max(0.0f, dot(normal, lightDir));

        finalColor += materialColor * lightColor * factor * nDotL;
    }

    gOutput[launchIndex] = float4(finalColor.rgb, 1.0f); 

    // DEPTH
	//gOutput[launchIndex] = float4(depth, 0.0f, 0.0f, 1.0f);

    // UV
	//gOutput[launchIndex] = float4(uv.xy, 0.0f, 1.0f);
    
    // FLAT WHITE
	//gOutput[launchIndex] = float4(1.0f, 1.0f, 1.0f, 1.0f);
}
