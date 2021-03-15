#ifndef INLINERTCOMPUTETASK_H
#define INLINERTCOMPUTETASK_H

#include "ComputeTask.h"
class ShaderResourceView;
class PingPongResource;

class InlineRTComputeTask : public ComputeTask
{
public:
	InlineRTComputeTask(
		ID3D12Device5* device,
		RootSignature* rootSignature,
		std::vector<std::pair<std::wstring, std::wstring>> csNamePSOName,
		COMMAND_INTERFACE_TYPE interfaceType);
	virtual ~InlineRTComputeTask();

	void Execute();
private:
};

#endif