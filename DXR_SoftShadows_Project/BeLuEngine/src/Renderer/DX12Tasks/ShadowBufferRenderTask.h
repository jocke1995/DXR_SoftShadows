#ifndef SHADOWBUFFERRENDERTASK_H
#define SHADOWBUFFERRENDERTASK_H

#include "RenderTask.h"

class PingPongResource;

class ShadowBufferRenderTask : public RenderTask
{
public:
	ShadowBufferRenderTask(ID3D12Device5* device,
		RootSignature* rootSignature,
		const std::wstring& VSName, const std::wstring& PSName,
		std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*>* gpsds,
		const std::wstring& psoName);
	~ShadowBufferRenderTask();

	void SetCommandInterface(CommandInterface* inter);
	void SetFullScreenQuadMesh(Mesh* fsq);

	void SetPingPongLightResources(int num, PingPongResource** ppPingPong);
	void SetHeapOffsets(unsigned int DhIndexSoftShadowsUAV, unsigned int DhIndexSoftShadowsBuffer);

	void CreateSlotInfo();

	void Execute() override final;

private:
	CommandInterface* m_pCommandInterfaceTemp = nullptr;
	Mesh* m_pFullScreenQuadMesh = nullptr;

	// CB_PER_OBJECT
	Resource* m_CbPerObjectUploadResource = nullptr;
	size_t m_NumIndices;
	SlotInfo m_SlotInfo;

	PingPongResource** m_ppTargetPingPongs = nullptr;
	int m_NumLights = -1;
	unsigned int m_DhIndexSoftShadowsUAV = 0;
	unsigned int m_DhIndexSoftShadowsBuffer = 0;
};

#endif