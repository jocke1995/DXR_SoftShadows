#ifndef UNORDEREDACCESSVIEW_H
#define UNORDEREDACCESSVIEW_H

#include "Descriptor.h"

class UnorderedAccessView : public Descriptor
{
public:
	UnorderedAccessView(
		ID3D12Device5* device,
		DescriptorHeap* descriptorHeap_CBV_UAV_SRV,
		D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc,
		Resource* resource);
	virtual ~UnorderedAccessView();

private:

	void createUnorderedAccessView(
		ID3D12Device5* device,
		DescriptorHeap* descriptorHeap_CBV_UAV_SRV,
		D3D12_UNORDERED_ACCESS_VIEW_DESC* desc);
};

#endif