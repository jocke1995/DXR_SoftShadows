#ifndef DESCRIPTORHEAP_H
#define DESCRIPTORHEAP_H

// DX12 Forward Declarations (But cant because using non-pointers)
#include "d3dx12.h"

enum class DESCRIPTOR_HEAP_TYPE
{
	CBV_UAV_SRV,
	RTV,
	DSV,
	NUM_DESCRIPTORHEAP_TYPES
};

class DescriptorHeap
{
public:
	DescriptorHeap(ID3D12Device5* device, DESCRIPTOR_HEAP_TYPE type);
	virtual ~DescriptorHeap();

	void SetCPUGPUHeapStart();
	void IncrementDescriptorHeapIndex();

	unsigned int GetNextDescriptorHeapIndex(unsigned int increment = 0);
	const D3D12_DESCRIPTOR_HEAP_DESC* GetDesc() const;
	ID3D12DescriptorHeap* GetID3D12DescriptorHeap() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHeapAt(unsigned int descriptorIndex);
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHeapAt(unsigned int descriptorIndex);
	const unsigned int GetHandleIncrementSize() const;

private:
	ID3D12DescriptorHeap* m_pDescriptorHeap = nullptr;
	unsigned int m_DescriptorHeapIndex = 0;

	D3D12_DESCRIPTOR_HEAP_DESC m_Desc = {};

	D3D12_CPU_DESCRIPTOR_HANDLE m_CPUHeapStart;
	D3D12_GPU_DESCRIPTOR_HANDLE m_GPUHeapStart;

	D3D12_CPU_DESCRIPTOR_HANDLE m_CPUHeapAt;
	D3D12_GPU_DESCRIPTOR_HANDLE m_GPUHeapAt;

	unsigned int m_HandleIncrementSize;
};

#endif