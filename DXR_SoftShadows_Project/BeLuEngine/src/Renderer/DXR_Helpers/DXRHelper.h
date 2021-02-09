#ifndef DXRHELPER_H

#include <d3d12.h>

// #DXR
struct AccelerationStructureBuffers
{
    ID3D12Resource1* pScratch;      // Scratch memory for AS builder
    ID3D12Resource1* pResult;       // Where the AS is
    ID3D12Resource1* pInstanceDesc; // Hold the matrices of the instances
};

#include "BottomLevelASGenerator.h"
#include "RaytracingPipelineGenerator.h"
#include "RootSignatureGenerator.h"
#include "TopLevelASGenerator.h"
#include "ShaderBindingTableGenerator.h"

#endif