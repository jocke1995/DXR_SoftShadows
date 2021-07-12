#include "stdafx.h"
#include "Descriptor.h"

#include "Resource.h"
#include "../DescriptorHeap.h"

Descriptor::Descriptor(DescriptorHeap* descriptorHeap, Resource* resource)
{
    m_pResource = resource;
    m_DescriptorHeapIndex = descriptorHeap->GetNextDescriptorHeapIndex(1);
}

Descriptor::~Descriptor()
{
}

const Resource* const Descriptor::GetResource() const
{
    return m_pResource;
}

const unsigned int Descriptor::GetDescriptorHeapIndex() const
{
    return m_DescriptorHeapIndex;
}
