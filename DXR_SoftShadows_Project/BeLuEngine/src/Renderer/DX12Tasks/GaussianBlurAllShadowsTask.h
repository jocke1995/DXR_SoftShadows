#ifndef GAUSSIANBLURALLSHADOWSTASK_H
#define GAUSSIANBLURALLSHADOWSTASK_H

#include "ComputeTask.h"
class ShaderResourceView;
class PingPongResource;
class ID3D12Resource;
class DescriptorHeap;

class GaussianBlurAllShadowsTask : public ComputeTask
{
public:
	GaussianBlurAllShadowsTask(
		ID3D12Device5* device,
		RootSignature* rootSignature,
		DescriptorHeap* dHeap_CBV_SRV_UAV,
		std::vector<std::pair< std::wstring, std::wstring>> csNamePSOName,
		COMMAND_INTERFACE_TYPE interfaceType,
		unsigned int screenWidth, unsigned int screenHeight,
		unsigned int FLAG_THREAD
	);
	virtual ~GaussianBlurAllShadowsTask();

	void SetPingPongResorcesToBlur(int num, PingPongResource** targets);

	// Examesarbete temp:
	void SetCommandInterface(CommandInterface* inter);

	void Execute();
private:
	CommandInterface* m_pCommandInterfaceTemp = nullptr;

	void setBlurPingPongResource(PingPongResource* target);

	void createTempResource(ID3D12Device5* device, unsigned int width, unsigned int height, DXGI_FORMAT format);
	void createTempPingPongResource(ID3D12Device5* device, DescriptorHeap* dHeap, DXGI_FORMAT format);

	int m_NumPingPongResourcesToBlur = -1;
	PingPongResource** m_ppTargetsToBlur = nullptr;

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