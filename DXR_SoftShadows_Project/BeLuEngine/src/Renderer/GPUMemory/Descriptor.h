#ifndef VIEW_H
#define VIEW_H

class Resource;
class DescriptorHeap;

class Descriptor
{
public:
	Descriptor(DescriptorHeap* descriptorHeap_CBV_UAV_SRV, Resource* resource);
	virtual ~Descriptor();

	const Resource* const GetResource() const;
	const unsigned int GetDescriptorHeapIndex() const;
protected:
	Resource* m_pResource = nullptr;
	unsigned int m_DescriptorHeapIndex = 0;
};

#endif