#ifndef TAARENDERTASK_H
#define TAARENDERTASK_H

#include "RenderTask.h"

class PingPongResource;

class TAARenderTask : public RenderTask
{
public:
	TAARenderTask(ID3D12Device5* device,
		RootSignature* rootSignature,
		const std::wstring& VSName, const std::wstring& PSName,
		std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*>* gpsds,
		const std::wstring& psoName);
	~TAARenderTask();

	void SetCommandInterface(CommandInterface* inter);
	void SetFullScreenQuadMesh(Mesh* fsq);

	void SetTAAPingPongResource(PingPongResource* TAAPingPong);
	void SetCurrFrameUAVIndex(unsigned int index);

	void CreateSlotInfo();

	void Execute() override final;

private:
	CommandInterface* m_pCommandInterfaceTemp = nullptr;
	Mesh* m_pFullScreenQuadMesh = nullptr;
	PingPongResource* m_pTAAPingPong = nullptr;
	unsigned int m_pCurrFrameUAVIndex = 0;

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