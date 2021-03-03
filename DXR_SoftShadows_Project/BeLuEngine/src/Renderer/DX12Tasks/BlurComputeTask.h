#ifndef BRIGHTBLURTASK_H
#define BRIGHTBLURTASK_H

#include "ComputeTask.h"
class ShaderResourceView;
class PingPongResource;
class ID3D12Resource;

class BlurComputeTask : public ComputeTask
{
public:
	BlurComputeTask(
		ID3D12Device5* device,
		RootSignature* rootSignature,
		std::vector<std::pair<std::wstring, std::wstring>> csNamePSOName,
		COMMAND_INTERFACE_TYPE interfaceType,
		const PingPongResource* Bloom0_RESOURCE,
		const PingPongResource* Bloom1_RESOURCE,
		unsigned int screenWidth, unsigned int screenHeight,
		unsigned int FLAG_THREAD
		);
	virtual ~BlurComputeTask();

	void SetBlurTarget(ID3D12Resource* target);

	void Execute();
private:
	ID3D12Resource* m_pTargetResource = nullptr;

	std::array<const PingPongResource*, 2> m_PingPongResources;
	unsigned int m_HorizontalThreadGroupsX;
	unsigned int m_HorizontalThreadGroupsY;
	unsigned int m_VerticalThreadGroupsX;
	unsigned int m_VerticalThreadGroupsY;
	const unsigned int m_ThreadsPerGroup = 256;

	DescriptorHeapIndices m_DhIndices = {};
};

#endif