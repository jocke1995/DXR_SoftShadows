#ifndef GBUFFERRENDERTASK_H
#define GBUFFERRENDERTASK_H

#include "RenderTask.h"

class GBufferRenderTask : public RenderTask
{
public:
	GBufferRenderTask(ID3D12Device5* device,
		RootSignature* rootSignature, 
		const std::wstring& VSName, const std::wstring& PSName,
		std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*>* gpsds, 
		const std::wstring& psoName);
	~GBufferRenderTask();

	void Execute() override final;

private:
	void drawRenderComponent(
		RenderComponent* rc,
		const DirectX::XMMATRIX* viewProjTransposed,
		ID3D12GraphicsCommandList5* cl);
};

#endif