#include "stdafx.h"
#include "MergeLightningRenderTask.h"

// DX12 Specifics
#include "../Camera/BaseCamera.h"
#include "../CommandInterface.h"
#include "../DescriptorHeap.h"
#include "../GPUMemory/GPUMemory.h"
#include "../PipelineState/PipelineState.h"
#include "../RenderView.h"
#include "../RootSignature.h"
#include "../SwapChain.h"

// Model info
#include "../Model/Mesh.h"
#include "../Model/Transform.h"

MergeLightningRenderTask::MergeLightningRenderTask(ID3D12Device5* device, RootSignature* rootSignature, const std::wstring& VSName, const std::wstring& PSName, std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*>* gpsds, const std::wstring& psoName)
	: RenderTask(device, rootSignature, VSName, PSName, gpsds, psoName)
{
	m_CbPerObjectUploadResource = new Resource(
		device,
		sizeof(CB_PER_OBJECT_STRUCT),
		RESOURCE_TYPE::UPLOAD,
		L"CB_PER_OBJECT_UPLOAD_RESOURCE_ShadowBufferTask");
	m_SlotInfo = {};
}

MergeLightningRenderTask::~MergeLightningRenderTask()
{
	delete m_CbPerObjectUploadResource;
}

void MergeLightningRenderTask::SetCommandInterface(CommandInterface* inter)
{
	m_pCommandInterfaceTemp = inter;
}

void MergeLightningRenderTask::SetFullScreenQuadMesh(Mesh* fsq)
{
	m_pFullScreenQuadMesh = fsq;
}

void MergeLightningRenderTask::CreateSlotInfo()
{
	// Mesh
	m_NumIndices = m_pFullScreenQuadMesh->GetNumIndices();
	m_SlotInfo.vertexDataIndex = m_pFullScreenQuadMesh->m_pVertexBufferSRV->GetDescriptorHeapIndex();

	// Textures
	// This is copy pasta from MergeRenderTask:
	// The descriptorHeapIndices for the SRVs are currently put inside the textureSlots inside SlotInfo
	//m_SlotInfo.textureAlbedo = m_SRVIndices[0];	// Blurred srv
}

void MergeLightningRenderTask::Execute()
{
	ID3D12CommandAllocator* commandAllocator = m_pCommandInterfaceTemp->GetCommandAllocator(m_CommandInterfaceIndex);
	ID3D12GraphicsCommandList5* commandList = m_pCommandInterfaceTemp->GetCommandList(m_CommandInterfaceIndex);

	//m_pCommandInterfaceTemp->Reset(m_CommandInterfaceIndex);

	// Get renderTarget
	const RenderTargetView* swapChainRenderTarget = m_pSwapChain->GetRTV(m_BackBufferIndex);

	DescriptorHeap* descriptorHeap_CBV_UAV_SRV = m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV];
	ID3D12DescriptorHeap* d3d12DescriptorHeap = descriptorHeap_CBV_UAV_SRV->GetID3D12DescriptorHeap();
	commandList->SetDescriptorHeaps(1, &d3d12DescriptorHeap);
	commandList->SetPipelineState(m_PipelineStates[0]->GetPSO());
	commandList->SetGraphicsRootSignature(m_pRootSig);

	commandList->OMSetRenderTargets(0, nullptr, false, nullptr);

	// use viewport from 0 -> 1
	const D3D12_VIEWPORT* viewPort = swapChainRenderTarget->GetRenderView()->GetViewPort();
	const D3D12_RECT* rect = swapChainRenderTarget->GetRenderView()->GetScissorRect();
	commandList->RSSetViewports(1, viewPort);
	commandList->RSSetScissorRects(1, rect);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


	// Draw a fullscreen quad 
	DirectX::XMMATRIX identityMatrix = DirectX::XMMatrixIdentity();

	// Create a CB_PER_OBJECT struct
	CB_PER_OBJECT_STRUCT perObject = { identityMatrix, identityMatrix, m_SlotInfo };

	// Temp, should not SetData here
	m_CbPerObjectUploadResource->SetData(&perObject);
	commandList->SetGraphicsRootConstantBufferView(RS::CB_PER_OBJECT_CBV, m_CbPerObjectUploadResource->GetGPUVirtualAdress());

	commandList->IASetIndexBuffer(m_pFullScreenQuadMesh->GetIndexBufferView());

	Resource* GBufferNormalResource = const_cast<Resource*>(m_Resources["GBufferNormal"]);

	// Depth
	auto transition = CD3DX12_RESOURCE_BARRIER::Transition(
		m_pDepthStencil->GetDefaultResource()->GetID3D12Resource1(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,		// StateBefore
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);	// StateAfter
	commandList->ResourceBarrier(1, &transition);

	// Normal
	transition = CD3DX12_RESOURCE_BARRIER::Transition(
		GBufferNormalResource->GetID3D12Resource1(),
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,		// StateBefore
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);	// StateAfter
	commandList->ResourceBarrier(1, &transition);

	commandList->DrawIndexedInstanced(m_NumIndices, 1, 0, 0, 0);

	// Depth
	transition = CD3DX12_RESOURCE_BARRIER::Transition(
		m_pDepthStencil->GetDefaultResource()->GetID3D12Resource1(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,		// StateBefore
		D3D12_RESOURCE_STATE_DEPTH_WRITE);	// StateAfter
	commandList->ResourceBarrier(1, &transition);

	// Normal
	transition = CD3DX12_RESOURCE_BARRIER::Transition(
		GBufferNormalResource->GetID3D12Resource1(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,		// StateBefore
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);	// StateAfter
	commandList->ResourceBarrier(1, &transition);

}

void MergeLightningRenderTask::SetHeapOffsets(unsigned int shadowBufferOffset)
{
	m_SoftShadowBufferOffset = shadowBufferOffset;
}
