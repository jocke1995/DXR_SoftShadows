#include "stdafx.h"
#include "CopyTask.h"

// DX12 Specifics
#include "../CommandInterface.h"
#include "../GPUMemory/GPUMemory.h"

CopyTask::CopyTask(ID3D12Device5* device, COMMAND_INTERFACE_TYPE interfaceType)
	:DX12Task(device, interfaceType)
{
}

CopyTask::~CopyTask()
{

}

void CopyTask::Submit(std::tuple<Resource*, Resource*, const void*>* Upload_Default_Data)
{
	m_UploadDefaultData.push_back(*Upload_Default_Data);
}

void CopyTask::SubmitContinousMemory(ShaderResource_ContinousMemory* dataVec)
{
	m_ContinousMemoryToUploadData.push_back(dataVec);
}

void CopyTask::copyResource(
	ID3D12GraphicsCommandList5* commandList,
	Resource* uploadResource, Resource* defaultResource,
	const void* data)
{
	// Set the data into the upload heap
	uploadResource->SetData(data);

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		defaultResource->GetID3D12Resource1(),
		D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_STATE_COPY_DEST));

	// To Defaultheap from Uploadheap
	commandList->CopyResource(
		defaultResource->GetID3D12Resource1(),	// Receiever
		uploadResource->GetID3D12Resource1());	// Sender

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		defaultResource->GetID3D12Resource1(),
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_COMMON));
}

void CopyTask::copyResourceAppend(
	ID3D12GraphicsCommandList5* commandList,
	Resource* uploadResource, Resource* defaultResource,
	const std::vector<std::pair<const void*, unsigned int>>& dataVec)
{
	// Set the data into the upload heap
	uploadResource->SetDataAppend(dataVec);

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		defaultResource->GetID3D12Resource1(),
		D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_STATE_COPY_DEST));

	// To Defaultheap from Uploadheap
	commandList->CopyResource(
		defaultResource->GetID3D12Resource1(),	// Receiever
		uploadResource->GetID3D12Resource1());	// Sender

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		defaultResource->GetID3D12Resource1(),
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_COMMON));
}
