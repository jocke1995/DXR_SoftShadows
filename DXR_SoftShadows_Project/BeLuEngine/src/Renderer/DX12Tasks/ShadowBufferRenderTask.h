#ifndef SHADOWBUFFERRENDERTASK_H
#define SHADOWBUFFERRENDERTASK_H

#include "RenderTask.h"

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

	void Execute() override final;

private:
	CommandInterface* m_pCommandInterfaceTemp = nullptr;
};

#endif