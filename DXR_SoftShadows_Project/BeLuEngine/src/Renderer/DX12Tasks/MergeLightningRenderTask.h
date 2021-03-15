#ifndef MERGELIGHTNINGRENDERTASK_H
#define MERGELIGHTNINGRENDERTASK_H

#include "RenderTask.h"

class MergeLightningRenderTask : public RenderTask
{
public:
	MergeLightningRenderTask(ID3D12Device5* device,
		RootSignature* rootSignature,
		const std::wstring& VSName, const std::wstring& PSName,
		std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*>* gpsds,
		const std::wstring& psoName);
	~MergeLightningRenderTask();

	void SetCommandInterface(CommandInterface* inter);
	void SetFullScreenQuadMesh(Mesh* fsq);

	void CreateSlotInfo();

	void Execute() override final;

	void SetHeapOffsets(unsigned int shadowBufferOffset);

private:
	CommandInterface* m_pCommandInterfaceTemp = nullptr;
	Mesh* m_pFullScreenQuadMesh = nullptr;

	// CB_PER_OBJECT
	Resource* m_CbPerObjectUploadResource = nullptr;
	size_t m_NumIndices;
	SlotInfo m_SlotInfo;

	// heap offsets
	unsigned int m_SoftShadowBufferOffset = 0;
};

#endif