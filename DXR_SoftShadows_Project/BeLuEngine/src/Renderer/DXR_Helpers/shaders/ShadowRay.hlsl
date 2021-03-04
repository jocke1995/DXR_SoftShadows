#include "Common.hlsl"

// Maybe not needed?
//[shader("closesthit")]
//void ShadowClosestHit(inout ShadowHitInfo hit : SV_RayPayload)
//{
//    hit.isHit = true;
//}

[shader("miss")]
void ShadowMiss(inout ShadowHitInfo hit : SV_RayPayload)
{
    hit.isHit = false;
}