#include "stdafx.h"
#include "CopyPerFrameTask.h"

#include "../GPUMemory/GPUMemory.h"
// DX12 Specifics
#include "../CommandInterface.h"

CopyPerFrameTask::CopyPerFrameTask(ID3D12Device5* device, COMMAND_INTERFACE_TYPE interfaceType)
	:CopyTask(device, interfaceType)
{

}

CopyPerFrameTask::~CopyPerFrameTask()
{

}

void CopyPerFrameTask::ClearSpecific(const Resource* uploadResource)
{
	unsigned int i = 0;

	// Loop through all copyPerFrame tasks
	for (auto& tuple : m_UploadDefaultData)
	{
		if (std::get<0>(tuple) == uploadResource)
		{
			// Remove
			m_UploadDefaultData.erase(m_UploadDefaultData.begin() + i);
			return; // FOUND
		}
		i++;
	}

	// two arrays in copyPerFrameTask
	for (ShaderResource_ContinousMemory* memToUpload : m_ContinousMemoryToUploadData)
	{
		if (memToUpload->shaderResource->GetUploadResource() == uploadResource)
		{
			// Remove
			m_ContinousMemoryToUploadData.erase(m_ContinousMemoryToUploadData.begin() + i);
			return; // FOUND
		}
		i++;
	}
}

void CopyPerFrameTask::Clear()
{
	m_UploadDefaultData.clear();
	m_ContinousMemoryToUploadData.clear();
}

void CopyPerFrameTask::Execute()
{
	ID3D12CommandAllocator* commandAllocator = m_pCommandInterface->GetCommandAllocator(m_CommandInterfaceIndex);
	ID3D12GraphicsCommandList5* commandList = m_pCommandInterface->GetCommandList(m_CommandInterfaceIndex);

	m_pCommandInterface->Reset(m_CommandInterfaceIndex);

	for (auto& tuple : m_UploadDefaultData)
	{
		copyResource(
			commandList,
			std::get<0>(tuple),		// UploadHeap
			std::get<1>(tuple),		// DefaultHeap
			std::get<2>(tuple));	// Data
	}

	// Loop through the copyTasks where there are multiple calls to SetData. So when there are more then one kind of type in a resource Example: (rawBuffer for lights)
	for (ShaderResource_ContinousMemory* memToUpload : m_ContinousMemoryToUploadData)
	{
		copyResourceAppend(
			commandList,
			memToUpload->shaderResource->GetUploadResource(),
			memToUpload->shaderResource->GetDefaultResource(),
			memToUpload->dataSizeVec);
	}

	commandList->Close();
}
