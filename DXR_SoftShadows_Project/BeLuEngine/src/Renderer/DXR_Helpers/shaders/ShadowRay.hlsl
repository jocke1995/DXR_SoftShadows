#include "Common.hlsl"

//[shader("closesthit")]
//void ShadowClosestHit(inout ShadowHitInfo hit : SV_RayPayload, in BuiltInTriangleIntersectionAttributes attribs)
//{
//    hit.isHit = true;
//}

[shader("miss")]
void ShadowMiss(inout ShadowHitInfo hit : SV_RayPayload)
{
    hit.isHit = false;
}