#include "Common.hlsl"

#include "hlslhelpers.hlsl"


// Raytracing acceleration structure, accessed as a SRV
RaytracingAccelerationStructure SceneBVH : register(t0, space2);

[shader("closesthit")] 
void ClosestHit(inout HitInfo payload, Attributes attrib) 
{
	float3 barycentrics = 
    float3(1.f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);
	
	const float3 A = float3(1, 0, 0);
	const float3 B = float3(0, 1, 0);
	const float3 C = float3(0, 0, 1);
	
	float3 materialColor = float3(1.0f, 1.0f, 1.0f);

	//if (InstanceID() == 1)
	//{
		materialColor = A* barycentrics.x + B * barycentrics.y + C * barycentrics.z;
	//}
	
	
	uint2 launchIndex = DispatchRaysIndex();
	float2 dims = float2(DispatchRaysDimensions().xy);
	// From 0 to 1 ----> -1 to 1
	float2 d = ((launchIndex.xy + 0.5f) / dims.xy);
	
	
    float3 lightPos = float3(425.900238f, 666.148193f, -98.189651f);
	
	// Testing light pos
	lightPos = float3(0, 50, 0);
	
	float lightRadius = 1.0; // To low radius => coneAngle not accurate enough
    
    // Find the world - space hit position
    float3 worldOrigin = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
    
    float3 lightDir = normalize(lightPos - worldOrigin);
	
	float3 perpL = cross(lightDir, float3(0.f, 1.0f, 0.f));
	// Handle case where L = up -> perpL should then be (1,0,0)
	if (all(perpL == 0.0f))
	{
		perpL.x = 1.0;
	}
	
	// Use perpL to get a vector from worldPosition to the edge of the light sphere
	float3 toLightEdge = normalize((lightPos + perpL * lightRadius) - worldOrigin);
	
	// Angle between L and toLightEdge. Used as the cone angle when sampling shadow rays
	float coneAngle = acos(dot(lightDir, toLightEdge)) * 2;
	
	// Init random floats
	uint frameSeed = cbPerFrame.frameCounter + 200000;
	uint seed = initRand( frameSeed * d.x , frameSeed * d.y);
	
	
	float sumFactor = 0;
	int spp = 1;
	
	int i;
	for(i = 0; i < spp; i++)
	{
	
	float3 randDir = getConeSample(seed, lightDir, coneAngle);
	
    // Fire a shadow ray. The direction is hard-coded here, but can be fetched
    // from a constant-buffer
    RayDesc ray;
    ray.Origin = worldOrigin;
    ray.Direction = randDir;
    ray.TMin = 0.01;
    ray.TMax = distance(lightPos, ray.Origin);
    
    // Initialize the ray payload
    ShadowHitInfo shadowPayload;
    shadowPayload.isHit = true;
	
    // Trace the ray
    TraceRay(
        // Acceleration structure
        SceneBVH,
        // Flags can be used to specify the behavior upon hitting a surface
        RAY_FLAG_SKIP_CLOSEST_HIT_SHADER | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH,
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
        1,
        // Ray information to trace
        ray,
        // Payload associated to the ray, which will be used to communicate
        // between the hit/miss shaders and the raygen
        shadowPayload);
		
		float factor = shadowPayload.isHit ? 0.0 : 1.0;
		sumFactor += factor;
	}
	
	sumFactor /= spp;

	payload.colorAndDistance = float4(materialColor * sumFactor, RayTCurrent());
}
