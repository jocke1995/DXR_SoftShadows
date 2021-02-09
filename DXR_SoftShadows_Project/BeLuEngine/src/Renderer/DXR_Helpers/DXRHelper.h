#ifndef DXRHELPER_H

#include <d3d12.h>
#include "../GPUMemory/Resource.h"

struct AccelerationStructureBuffers 
{
	Resource* scratch = nullptr;
	Resource* result = nullptr;
	Resource* instanceDesc = nullptr;    // Used only for top-level AS
	bool allowUpdate = false;
	void release() 
	{
		if (scratch) 
		{
			delete scratch;
			scratch = nullptr;
		}
		if (result) 
		{
			delete result;
			result = nullptr;
		}
		if (instanceDesc) 
		{
			delete instanceDesc;
			instanceDesc = nullptr;
		}
	}
};

#include "BottomLevelASGenerator.h"
#include "RaytracingPipelineGenerator.h"
#include "RootSignatureGenerator.h"
#include "TopLevelASGenerator.h"
#include "ShaderBindingTableGenerator.h"

#endif