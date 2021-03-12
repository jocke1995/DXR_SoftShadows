#include "stdafx.h"
#include "BlurAllShadowsTask.h"

#include "../CommandInterface.h"
#include"../RootSignature.h"
#include "../PipelineState/ComputeState.h"

#include "../GPUMemory/PingPongResource.h"
#include "../GPUMemory/ShaderResourceView.h"
#include "../GPUMemory/UnorderedAccessView.h"
#include "../GPUMemory/Resource.h"
#include "../DescriptorHeap.h"

BlurAllShadowsTask::BlurAllShadowsTask(
	ID3D12Device5* device,
	RootSignature* rootSignature,
	DescriptorHeap* dHeap_CBV_SRV_UAV,
	std::vector<std::pair< std::wstring, std::wstring>> csNamePSOName,
	COMMAND_INTERFACE_TYPE interfaceType,
	unsigned int screenWidth, unsigned int screenHeight,
	unsigned int FLAG_THREAD)
	:ComputeTask(device, rootSignature, csNamePSOName, interfaceType)
{
	auto dxgiFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
	createTempResource(device, screenWidth, screenHeight, dxgiFormat);
	createTempPingPongResource(device, dHeap_CBV_SRV_UAV, dxgiFormat);

	m_HorizontalThreadGroupsX = static_cast<unsigned int>(ceilf(static_cast<float>(screenWidth) / m_ThreadsPerGroup));
	m_HorizontalThreadGroupsY = screenHeight;

	m_VerticalThreadGroupsX = screenWidth;
	m_VerticalThreadGroupsY = m_HorizontalThreadGroupsX;
}

BlurAllShadowsTask::~BlurAllShadowsTask()
{
	delete m_pTempPingPongResource;
	delete m_pTempResource;
}

void BlurAllShadowsTask::SetPingPongResorcesToBlur(int num, PingPongResource** targets)
{
	m_NumPingPongResourcesToBlur = num;
	m_ppTargetsToBlur = targets;
}

void BlurAllShadowsTask::setBlurPingPongResource(PingPongResource* target)
{
	m_pTargetPingPongResource = target;
}

void BlurAllShadowsTask::SetCommandInterface(CommandInterface* inter)
{
	m_pCommandInterfaceTemp = inter;
}

void BlurAllShadowsTask::Execute()
{
	ID3D12CommandAllocator* commandAllocator = m_pCommandInterfaceTemp->GetCommandAllocator(m_CommandInterfaceIndex);
	ID3D12GraphicsCommandList5* commandList = m_pCommandInterfaceTemp->GetCommandList(m_CommandInterfaceIndex);

	//commandAllocator->Reset();
	//commandList->Reset(commandAllocator, NULL);

	commandList->SetComputeRootSignature(m_pRootSig);

	DescriptorHeap* descriptorHeap_CBV_UAV_SRV = m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV];
	ID3D12DescriptorHeap* d3d12DescriptorHeap = descriptorHeap_CBV_UAV_SRV->GetID3D12DescriptorHeap();
	commandList->SetDescriptorHeaps(1, &d3d12DescriptorHeap);

	commandList->SetComputeRootDescriptorTable(RS::dtUAV, descriptorHeap_CBV_UAV_SRV->GetGPUHeapAt(0));
	commandList->SetComputeRootDescriptorTable(RS::dtSRV, descriptorHeap_CBV_UAV_SRV->GetGPUHeapAt(0));

	// Blurs all texture
	for (unsigned int i = 0; i < m_NumPingPongResourcesToBlur; i++)
	{
		setBlurPingPongResource(m_ppTargetsToBlur[i]);

		// Descriptorheap indices for the textures to blur
		// Horizontal pass
		m_DhIndices.index0 = m_pTargetPingPongResource->GetSRV()->GetDescriptorHeapIndex();	// Read
		m_DhIndices.index1 = m_pTempPingPongResource->GetUAV()->GetDescriptorHeapIndex();	// Write
		// Vertical pass
		m_DhIndices.index2 = m_pTempPingPongResource->GetSRV()->GetDescriptorHeapIndex();	// Read
		m_DhIndices.index3 = m_pTargetPingPongResource->GetUAV()->GetDescriptorHeapIndex();	// Write
		// Send the indices to gpu
		commandList->SetComputeRoot32BitConstants(RS::RC_4, 4, &m_DhIndices, 0);

		// The resource to read (Resource Barrier)
		TransResourceState(commandList, const_cast<Resource*>(m_pTargetPingPongResource->GetSRV()->GetResource()),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		// The resource to write (Resource Barrier)
		TransResourceState(commandList, const_cast<Resource*>(m_pTempPingPongResource->GetUAV()->GetResource()),
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		// Blur horizontal
		commandList->SetPipelineState(m_PipelineStates[0]->GetPSO());
		commandList->Dispatch(m_HorizontalThreadGroupsX, m_HorizontalThreadGroupsY, 1);

		// The resource to write to (Resource Barrier)
		TransResourceState(commandList, const_cast<Resource*>(m_pTempPingPongResource->GetSRV()->GetResource()),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		// The resource to read (Resource Barrier)
		TransResourceState(commandList, const_cast<Resource*>(m_pTargetPingPongResource->GetUAV()->GetResource()),
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		// Blur vertical
		commandList->SetPipelineState(m_PipelineStates[1]->GetPSO());
		commandList->Dispatch(m_VerticalThreadGroupsX, m_VerticalThreadGroupsY, 1);

		TransResourceState(commandList, const_cast<Resource*>(m_pTargetPingPongResource->GetSRV()->GetResource()),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}
	

	//commandList->Close();
}

void BlurAllShadowsTask::createTempResource(ID3D12Device5* device, unsigned int width, unsigned int height, DXGI_FORMAT format)
{
	D3D12_RESOURCE_DESC desc = {};
	desc.DepthOrArraySize = 1;
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	// The backbuffer is actually DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, but sRGB
	// formats cannot be used with UAVs. For accuracy we should convert to sRGB
	// ourselves in the shader
	desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;

	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	desc.Width = width;
	desc.Height = height;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;

	// Init temp Resource
	m_pTempResource = new Resource(device, &desc, nullptr, L"BlurTaskTempDefaultResource", D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
}

void BlurAllShadowsTask::createTempPingPongResource(ID3D12Device5* device, DescriptorHeap* dHeap, DXGI_FORMAT format)
{
	// Create SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	// Create UAV
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = format;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

	m_pTempPingPongResource = new PingPongResource(m_pTempResource, device, dHeap, &srvDesc, &uavDesc);
}
