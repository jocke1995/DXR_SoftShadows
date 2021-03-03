#ifndef BRIGHTBLURTASK_H
#define BRIGHTBLURTASK_H

#include "ComputeTask.h"
class ShaderResourceView;
class PingPongResource;
class ID3D12Resource;
class DescriptorHeap;

class BlurComputeTask : public ComputeTask
{
public:
	BlurComputeTask(
			ID3D12Device5* device,
			RootSignature* rootSignature,
			DescriptorHeap* dHeap_CBV_SRV_UAV,
			std::vector<std::pair< std::wstring, std::wstring>> csNamePSOName,
			COMMAND_INTERFACE_TYPE interfaceType,
			unsigned int screenWidth, unsigned int screenHeight,
			unsigned int FLAG_THREAD
		);
	virtual ~BlurComputeTask();

	void SetBlurPingPongResource(PingPongResource* target);

	void Execute();
private:

	void createTempResource(ID3D12Device5* device, unsigned int width, unsigned int height, DXGI_FORMAT format);
	void createTempPingPongResource(ID3D12Device5* device, DescriptorHeap* dHeap, DXGI_FORMAT format);

	Resource* m_pTempResource = nullptr;
	PingPongResource* m_pTempPingPongResource = nullptr;
	PingPongResource* m_pTargetPingPongResource = nullptr;

	unsigned int m_HorizontalThreadGroupsX;
	unsigned int m_HorizontalThreadGroupsY;
	unsigned int m_VerticalThreadGroupsX;
	unsigned int m_VerticalThreadGroupsY;
	const unsigned int m_ThreadsPerGroup = 256;

	DescriptorHeapIndices m_DhIndices = {};
};

#endif