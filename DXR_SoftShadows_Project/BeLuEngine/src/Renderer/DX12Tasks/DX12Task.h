#ifndef DX12TASK_H
#define DX12TASK_H

class CommandInterface;
class Resource;
class DescriptorHeap;

enum COMMAND_INTERFACE_TYPE;
enum class DESCRIPTOR_HEAP_TYPE;
enum D3D12_RESOURCE_STATES;

// DX12 Forward Declarations
struct ID3D12GraphicsCommandList5;

// These renderTasks will execute on "all objects"
enum RENDER_TASK_TYPE
{
	DEPTH_PRE_PASS,
	GBUFFER_PASS,
	FORWARD_RENDER,
	NR_OF_RENDERTASKS
};

enum COMPUTE_TASK_TYPE
{
	BLUR,
	INLINE_RT,
	NR_OF_COMPUTETASKS
};

enum COPY_TASK_TYPE
{
	COPY_PER_FRAME,
	COPY_ON_DEMAND,
	NR_OF_COPYTASKS
};

class DX12Task
{
public:
	DX12Task(ID3D12Device5* device, COMMAND_INTERFACE_TYPE interfaceType);
	virtual ~DX12Task();

	static void SetBackBufferIndex(int backBufferIndex);
	static void SetCommandInterfaceIndex(int index);
	void SetDescriptorHeaps(std::map<DESCRIPTOR_HEAP_TYPE, DescriptorHeap*> dhs);

	void AddResource(std::string id, const Resource* resource);

	CommandInterface* const GetCommandInterface() const;

	virtual void Execute() = 0;
protected:
	std::map<DESCRIPTOR_HEAP_TYPE, DescriptorHeap*> m_DescriptorHeaps;
	std::map<std::string, const Resource*> m_Resources;

	CommandInterface* m_pCommandInterface = nullptr;
	inline static int m_BackBufferIndex = -1;
	inline static int m_CommandInterfaceIndex = -1;

	void TransResourceState(ID3D12GraphicsCommandList5* cl, Resource* resource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter);
};

#endif