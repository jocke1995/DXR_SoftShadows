#include "stdafx.h"
#include "ShadowBufferRenderTask.h"

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

ShadowBufferRenderTask::ShadowBufferRenderTask(ID3D12Device5* device, RootSignature* rootSignature, const std::wstring& VSName, const std::wstring& PSName, std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*>* gpsds, const std::wstring& psoName)
	: RenderTask(device, rootSignature, VSName, PSName, gpsds, psoName)
{
}

ShadowBufferRenderTask::~ShadowBufferRenderTask()
{
}

void ShadowBufferRenderTask::SetCommandInterface(CommandInterface* inter)
{
	m_pCommandInterfaceTemp = inter;
}

void ShadowBufferRenderTask::Execute()
{
	ID3D12CommandAllocator* commandAllocator = m_pCommandInterfaceTemp->GetCommandAllocator(m_CommandInterfaceIndex);
	ID3D12GraphicsCommandList5* commandList = m_pCommandInterfaceTemp->GetCommandList(m_CommandInterfaceIndex);
	m_pCommandInterfaceTemp->Reset(m_CommandInterfaceIndex);

	commandList->SetGraphicsRootSignature(m_pRootSig);

	DescriptorHeap* descriptorHeap_CBV_UAV_SRV = m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV];
	ID3D12DescriptorHeap* d3d12DescriptorHeap = descriptorHeap_CBV_UAV_SRV->GetID3D12DescriptorHeap();
	commandList->SetDescriptorHeaps(1, &d3d12DescriptorHeap);

	commandList->SetGraphicsRootDescriptorTable(RS::dtSRV, descriptorHeap_CBV_UAV_SRV->GetGPUHeapAt(0));

	DescriptorHeap* depthBufferHeap = m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::DSV];

	commandList->SetPipelineState(m_PipelineStates[0]->GetPSO());
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// TODO: Get Depth viewport, rightnow use swapchain since the view and rect is the same.
	const D3D12_VIEWPORT* viewPort = m_pSwapChain->GetRTV(0)->GetRenderView()->GetViewPort();
	const D3D12_RECT* rect = m_pSwapChain->GetRTV(0)->GetRenderView()->GetScissorRect();
	commandList->RSSetViewports(1, viewPort);
	commandList->RSSetScissorRects(1, rect);

	const DirectX::XMMATRIX* viewProjMatTrans = m_pCamera->GetViewProjectionTranposed();

	unsigned int index = m_pDepthStencil->GetDSV()->GetDescriptorHeapIndex();

	// Clear and set depth + stencil
	D3D12_CPU_DESCRIPTOR_HANDLE dsh = depthBufferHeap->GetCPUHeapAt(index);
	commandList->ClearDepthStencilView(dsh, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	commandList->OMSetRenderTargets(0, nullptr, false, &dsh);

	

	commandList->Close();
}
