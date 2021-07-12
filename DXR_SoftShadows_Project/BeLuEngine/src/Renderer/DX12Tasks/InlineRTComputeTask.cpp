#include "stdafx.h"
#include "InlineRTComputeTask.h"

#include "../CommandInterface.h"
#include"../RootSignature.h"

#include "../DescriptorHeap.h"

#include "../PipelineState/ComputeState.h"

InlineRTComputeTask::InlineRTComputeTask(
	ID3D12Device5* device,
	RootSignature* rootSignature,
	std::vector<std::pair< std::wstring, std::wstring>> csNamePSOName,
	COMMAND_INTERFACE_TYPE interfaceType)
	:ComputeTask(device, rootSignature, csNamePSOName)
{
	
}

InlineRTComputeTask::~InlineRTComputeTask()
{

}

void InlineRTComputeTask::Execute()
{
	//ID3D12CommandAllocator* commandAllocator = m_pCommandInterface->GetCommandAllocator(m_CommandInterfaceIndex);
	//ID3D12GraphicsCommandList5* commandList = m_pCommandInterface->GetCommandList(m_CommandInterfaceIndex);
	//
	//commandAllocator->Reset();
	//commandList->Reset(commandAllocator, NULL);
	//
	//commandList->SetComputeRootSignature(m_pRootSig);
	//
	//DescriptorHeap* descriptorHeap_CBV_UAV_SRV = m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV];
	//ID3D12DescriptorHeap* d3d12DescriptorHeap = descriptorHeap_CBV_UAV_SRV->GetID3D12DescriptorHeap();
	//commandList->SetDescriptorHeaps(1, &d3d12DescriptorHeap);
	//
	//commandList->SetComputeRootDescriptorTable(RS::dtUAV, descriptorHeap_CBV_UAV_SRV->GetGPUHeapAt(0));
	//commandList->SetComputeRootDescriptorTable(RS::dtSRV, descriptorHeap_CBV_UAV_SRV->GetGPUHeapAt(0));
	//commandList->SetComputeRootConstantBufferView(RS::CB_PER_SCENE, )
	//
	//// Send the indices to gpu
	////commandList->SetComputeRoot32BitConstants(RS::CB_INDICES_CONSTANTS, sizeof(DescriptorHeapIndices) / sizeof(UINT), &m_DhIndices, 0);
	//
	//
	//// Blur horizontal
	//commandList->SetPipelineState(m_PipelineStates[0]->GetPSO());
	//commandList->Dispatch(m_NumThreadGroupsX, m_NumThreadGroupsY, 1);
	//
	//
	//commandList->Close();
}
