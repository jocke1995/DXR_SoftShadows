#ifndef CONSTANTBUFFERVIEW_H
#define CONSTANTBUFFERVIEW_H

#include "Descriptor.h"

class ConstantBufferView : public Descriptor
{
public:
	ConstantBufferView(
		ID3D12Device5* device,
		DescriptorHeap* descriptorHeap_CBV_UAV_SRV,
		D3D12_CONSTANT_BUFFER_VIEW_DESC* cbvDesc,
		Resource* resource);

	virtual ~ConstantBufferView();

private:
	void createConstantBufferView(
		ID3D12Device5* device,
		DescriptorHeap* descriptorHeap_CBV_UAV_SRV,
		D3D12_CONSTANT_BUFFER_VIEW_DESC* cbvDesc);
};

#endif

