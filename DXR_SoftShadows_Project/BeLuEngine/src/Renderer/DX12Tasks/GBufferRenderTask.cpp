#include "stdafx.h"
#include "GBufferRenderTask.h"

// DX12 Specifics
#include "../Camera/BaseCamera.h"
#include "../CommandInterface.h"
#include "../DescriptorHeap.h"
#include "../GPUMemory/GPUMemory.h"
#include "../PipelineState/PipelineState.h"
#include "../RenderView.h"
#include "../RootSignature.h"
#include "../SwapChain.h"
//#include "RenderTask.h"

// Model info
#include "../Renderer/Model/Transform.h"
#include "../Renderer/Model/Mesh.h"

GBufferRenderTask::GBufferRenderTask(
	ID3D12Device5* device,
	RootSignature* rootSignature,
	const std::wstring& VSName, const std::wstring& PSName,
	std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*>* gpsds,
	const std::wstring& psoName)
	:RenderTask(device, rootSignature, VSName, PSName, gpsds, psoName)
{
	
}

GBufferRenderTask::~GBufferRenderTask()
{
}

void GBufferRenderTask::SetCommandInterface(CommandInterface* inter)
{
	m_pTempCommandInterface = inter;
}

void GBufferRenderTask::Execute()
{
	ID3D12CommandAllocator* commandAllocator = m_pTempCommandInterface->GetCommandAllocator(0);
	ID3D12GraphicsCommandList5* commandList = m_pTempCommandInterface->GetCommandList(0);

	const RenderTargetView* rtv = m_RenderTargetViews["NormalRTV"];

	commandList->SetGraphicsRootSignature(m_pRootSig);
	
	DescriptorHeap* descriptorHeap_CBV_UAV_SRV = m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV];
	ID3D12DescriptorHeap* d3d12DescriptorHeap = descriptorHeap_CBV_UAV_SRV->GetID3D12DescriptorHeap();
	commandList->SetDescriptorHeaps(1, &d3d12DescriptorHeap);

	commandList->SetGraphicsRootDescriptorTable(RS::dtCBV, descriptorHeap_CBV_UAV_SRV->GetGPUHeapAt(0));
	commandList->SetGraphicsRootDescriptorTable(RS::dtSRV, descriptorHeap_CBV_UAV_SRV->GetGPUHeapAt(0));

	// Change state on Resource
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		rtv->GetResource()->GetID3D12Resource1(),
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET));

	DescriptorHeap* renderTargetHeap = m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::RTV];
	DescriptorHeap* depthBufferHeap  = m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::DSV];

	// RenderTargets
	const unsigned int gBufferNormalDHIndex = rtv->GetDescriptorHeapIndex();
	D3D12_CPU_DESCRIPTOR_HANDLE cdhGBufferNormal = renderTargetHeap->GetCPUHeapAt(gBufferNormalDHIndex);
	D3D12_CPU_DESCRIPTOR_HANDLE cdhs[] = { cdhGBufferNormal };

	// Depth
	D3D12_CPU_DESCRIPTOR_HANDLE dsh = depthBufferHeap->GetCPUHeapAt(m_pDepthStencil->GetDSV()->GetDescriptorHeapIndex());

	commandList->OMSetRenderTargets(1, cdhs, false, &dsh);

	float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	commandList->ClearRenderTargetView(cdhGBufferNormal, clearColor, 0, nullptr);

	const D3D12_VIEWPORT viewPortSwapChain = *rtv->GetRenderView()->GetViewPort();
	const D3D12_VIEWPORT viewPorts[1] = { viewPortSwapChain };

	const D3D12_RECT rectSwapChain = *rtv->GetRenderView()->GetScissorRect();
	const D3D12_RECT rects[1] = { rectSwapChain };

	commandList->RSSetViewports(1, viewPorts);
	commandList->RSSetScissorRects(1, rects);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	const DirectX::XMMATRIX* viewProjMatTrans = m_pCamera->GetViewProjectionTranposed();

	// This pair for m_RenderComponents will be used for model-outlining in case any model is picked.
	std::pair<component::ModelComponent*, component::TransformComponent*> outlinedModel = std::make_pair(nullptr, nullptr);

	// Draw for every Rendercomponent with stencil testing disabled
	commandList->SetPipelineState(m_PipelineStates[0]->GetPSO());
	for (int i = 0; i < m_RenderComponents->size(); i++)
	{
		drawRenderComponent(m_RenderComponents->at(i), viewProjMatTrans, commandList);
	}

	// Change state on Resource
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		rtv->GetResource()->GetID3D12Resource1(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));
}

void GBufferRenderTask::drawRenderComponent(
	RenderComponent* rc,
	const DirectX::XMMATRIX* viewProjTransposed,
	ID3D12GraphicsCommandList5* cl)
{
	component::ModelComponent	 * mc	= rc->mc;
	component::TransformComponent* tc	= rc->tc;

	// Draw for every m_pMesh the meshComponent has
	for (unsigned int i = 0; i < mc->GetNrOfMeshes(); i++)
	{
		Mesh* m = mc->GetMeshAt(i);
		unsigned int num_Indices = m->GetNumIndices();

		// TODO: change to default
		cl->SetGraphicsRootConstantBufferView(RS::CB_PER_OBJECT_CBV, rc->CB_PER_OBJECT_UPLOAD_RESOURCES[i]->GetGPUVirtualAdress());

		cl->IASetIndexBuffer(m->GetIndexBufferView());
		cl->DrawIndexedInstanced(num_Indices, 1, 0, 0, 0);
	}
}
