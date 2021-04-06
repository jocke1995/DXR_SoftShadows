#include "stdafx.h"
#include "DepthRenderTask.h"

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

DepthRenderTask::DepthRenderTask(ID3D12Device5* device, 
	RootSignature* rootSignature, 
	const std::wstring& VSName, const std::wstring& PSName,
	std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*>* gpsds, 
	const std::wstring& psoName)
	: RenderTask(device, rootSignature, VSName, PSName, gpsds, psoName)
{
}

DepthRenderTask::~DepthRenderTask()
{
}

void DepthRenderTask::Execute()
{
	ID3D12CommandAllocator* commandAllocator = m_pCommandInterface->GetCommandAllocator(m_CommandInterfaceIndex);
	ID3D12GraphicsCommandList5* commandList = m_pCommandInterface->GetCommandList(m_CommandInterfaceIndex);
	m_pCommandInterface->Reset(m_CommandInterfaceIndex);

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

	static bool updateMatrices = true;
	// Draw for every Rendercomponent
	for (int i = 0; i < m_RenderComponents->size(); i++)
	{
		drawRenderComponent(m_RenderComponents->at(i), viewProjMatTrans, commandList, updateMatrices);
	}

	updateMatrices = false;

	commandList->Close();
}

void DepthRenderTask::drawRenderComponent(
	RenderComponent* rc,
	const DirectX::XMMATRIX* viewProjTransposed,
	ID3D12GraphicsCommandList5* cl,
	bool updateMatrices)
{
	component::ModelComponent	 * mc = rc->mc;
	component::TransformComponent* tc = rc->tc;

	// Draw for every m_pMesh the meshComponent has
	for (unsigned int i = 0; i < mc->GetNrOfMeshes(); i++)
	{
		Mesh* m = mc->GetMeshAt(i);
		unsigned int num_Indices = m->GetNumIndices();
		const SlotInfo* info = mc->GetSlotInfoAt(i);

		Transform* transform = tc->GetTransform();
		DirectX::XMMATRIX* WTransposed = transform->GetWorldMatrixTransposed();
		DirectX::XMMATRIX WVPTransposed = (*viewProjTransposed) * (*WTransposed);

		// Create a CB_PER_OBJECT struct
		CB_PER_OBJECT_STRUCT perObject = { *WTransposed, WVPTransposed, *info };

#ifdef DIST
		// Temp, should not SetData here, Only does it here because it doesn't matter for the master thesis
		if (updateMatrices == true)
		{
#endif
			Resource* upl = rc->CB_PER_OBJECT_UPLOAD_RESOURCES[i];
			Resource* def = rc->CB_PER_OBJECT_DEFAULT_RESOURCES[i];
			upl->SetData(&perObject);

			cl->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
				def->GetID3D12Resource1(),
				D3D12_RESOURCE_STATE_COMMON,
				D3D12_RESOURCE_STATE_COPY_DEST));

			// To Defaultheap from Uploadheap
			cl->CopyResource(
				def->GetID3D12Resource1(),	// Receiever
				upl->GetID3D12Resource1());	// Sender

			cl->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
				def->GetID3D12Resource1(),
				D3D12_RESOURCE_STATE_COPY_DEST,
				D3D12_RESOURCE_STATE_COMMON));
#ifdef DIST
		}
#endif

		cl->SetGraphicsRootConstantBufferView(RS::CB_PER_OBJECT_CBV, rc->CB_PER_OBJECT_DEFAULT_RESOURCES[i]->GetGPUVirtualAdress());

		cl->IASetIndexBuffer(m->GetIndexBufferView());
		cl->DrawIndexedInstanced(num_Indices, 1, 0, 0, 0);
	}
}
