#ifndef FORWARDRENDERTASK_H
#define FORWARDRENDERTASK_H

#include "RenderTask.h"

class ForwardRenderTask : public RenderTask
{
public:
	ForwardRenderTask(ID3D12Device5* device, 
		RootSignature* rootSignature, 
		const std::wstring& VSName, const std::wstring& PSName,
		std::vector<PSO_STREAM*>* pso_streams,
		const std::wstring& psoName);
	~ForwardRenderTask();

	void Execute() override final;

private:
	void drawRenderComponent(
		RenderComponent* rc,
		const DirectX::XMMATRIX* viewProjTransposed,
		ID3D12GraphicsCommandList5* cl);
};

#endif