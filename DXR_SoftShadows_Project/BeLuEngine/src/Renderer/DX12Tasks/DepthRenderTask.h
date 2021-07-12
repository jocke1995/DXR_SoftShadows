#ifndef DEPTHRENDERTASK_H
#define DEPTHRENDERTASK_H

#include "RenderTask.h"

class DepthRenderTask : public RenderTask
{
public:
	DepthRenderTask(ID3D12Device5* device,
		RootSignature* rootSignature,
		const std::wstring& VSName, const std::wstring& PSName,
		std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*>* gpsds,
		const std::wstring& psoName);
	~DepthRenderTask();

	// Examesarbete temp:
	void SetCommandInterface(CommandInterface* inter);

	void Execute() override final;

private:
	void drawRenderComponent(
		RenderComponent* rc,
		const DirectX::XMMATRIX* viewProjTransposed,
		ID3D12GraphicsCommandList5* cl,
		bool updateMatrices);

	CommandInterface* m_pTempCommandInterface = nullptr;
};

#endif