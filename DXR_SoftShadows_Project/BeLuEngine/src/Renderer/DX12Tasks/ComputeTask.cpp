#include "stdafx.h"
#include "ComputeTask.h"

// DX12 Specifics
#include "../PipelineState/ComputeState.h"
#include "../RootSignature.h"

ComputeTask::ComputeTask(
	ID3D12Device5* device,
	RootSignature* rootSignature,
	std::vector<std::pair<std::wstring, std::wstring>> csNamePSOName,
	COMMAND_INTERFACE_TYPE interfaceType)
	:DX12Task(device, interfaceType)
{
	for (auto& pair : csNamePSOName)
	{
		m_PipelineStates.push_back(new ComputeState(device, rootSignature, pair.first, pair.second));
	}

	m_pRootSig = rootSignature->GetRootSig();
}

ComputeTask::~ComputeTask()
{
	for (auto cso : m_PipelineStates)
	{
		delete cso;
	}
	
}
