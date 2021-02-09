#ifndef RENDERER_H
#define RENDERER_H

// Misc
class ThreadPool;
class Window;

#include "DXR_Helpers/DXRHelper.h"
// Renderer Engine
class RootSignature;
class SwapChain;
class RenderTargetView;
class ViewPool;
class BoundingBoxPool;
class DescriptorHeap;
class Mesh;
class Texture;
class Model;
class Resource;
class CommandInterface;

// GPU Resources
class ConstantBuffer;
class ShaderResource;
class UnorderedAccess;
class DepthStencil;
class Resource;

// Enums
enum COMMAND_INTERFACE_TYPE;
enum class DESCRIPTOR_HEAP_TYPE;

// techniques
class ShadowInfo;
class MousePicker;
class Bloom;

// ECS
class Scene;
class Light;

// Graphics
class RenderTask;
class WireframeRenderTask;
class OutliningRenderTask;
class BaseCamera;
class Material;
struct RenderComponent;
struct ID3D12Resource1;

// Copy
class CopyTask;

// Compute
class ComputeTask;

// DX12 Forward Declarations
struct ID3D12CommandQueue;
struct ID3D12CommandList;
struct ID3D12Fence1;
struct ID3D12Device5;

// ECS
class Entity;
namespace component
{
	class ModelComponent;
	class TransformComponent;
	class CameraComponent;
	class DirectionalLightComponent;
	class PointLightComponent;
	class SpotLightComponent;
}

// Events
struct WindowChange;
struct WindowSettingChange;

class Renderer
{
public:
	static Renderer& GetInstance();
	virtual ~Renderer();

	// Scene
	Scene* const GetActiveScene() const;

	Window* const GetWindow() const;

	// Call once
	void InitD3D12(Window* window, HINSTANCE hInstance, ThreadPool* threadPool);

	// Call each frame
	void Update(double dt);
	void SortObjects();
	void Execute();
	void ExecuteDXR();

	// Render inits, these functions are called by respective components through SetScene to prepare for drawing
	void InitModelComponent(component::ModelComponent* component);
	void InitDirectionalLightComponent(component::DirectionalLightComponent* component);
	void InitPointLightComponent(component::PointLightComponent* component);
	void InitSpotLightComponent(component::SpotLightComponent* component);
	void InitCameraComponent(component::CameraComponent* component);

	void UnInitModelComponent(component::ModelComponent* component);
	void UnInitDirectionalLightComponent(component::DirectionalLightComponent* component);
	void UnInitPointLightComponent(component::PointLightComponent* component);
	void UnInitSpotLightComponent(component::SpotLightComponent* component);
	void UnInitCameraComponent(component::CameraComponent* component);

	void OnResetScene();

private:
	friend class BeLuEngine;
	friend class SceneManager;
	Renderer();

	// For control of safe release of DirectX resources
	void deleteRenderer();

	// SubmitToCodt functions
	void submitToCodt(std::tuple<Resource*, Resource*, const void*>* Upload_Default_Data);
	void submitModelToGPU(Model* model);
	void submitMeshToCodt(Mesh* mesh);
	void submitTextureToCodt(Texture* texture);

	//SubmitToCpft functions
	void submitToCpft(std::tuple<Resource*, Resource*, const void*>* Upload_Default_Data);
	void clearSpecificCpft(Resource* upload);

	DescriptorHeap* getCBVSRVUAVdHeap() const;

	ThreadPool* m_pThreadPool = nullptr;

	// Camera
	BaseCamera* m_pScenePrimaryCamera = nullptr;

	unsigned int m_FrameCounter = 0;

	// Window
	Window* m_pWindow;

	// Device
	ID3D12Device5* m_pDevice5 = nullptr;

	// CommandQueues
	std::map<COMMAND_INTERFACE_TYPE, ID3D12CommandQueue*> m_CommandQueues;

	// RenderTargets
	// Swapchain (inheriting from 'RenderTarget')
	SwapChain* m_pSwapChain = nullptr;
	
	// Bloom (includes rtv, uav and srv)
	Bloom* m_pBloomResources = nullptr;

	// Depthbuffer
	DepthStencil* m_pMainDepthStencil = nullptr;

	// Rootsignature
	RootSignature* m_pRootSignature = nullptr;

	// Picking
	MousePicker* m_pMousePicker = nullptr;
	Entity* m_pPickedEntity = nullptr;

	// Tasks
	std::vector<ComputeTask*> m_ComputeTasks;
	std::vector<CopyTask*>    m_CopyTasks;
	std::vector<RenderTask*>  m_RenderTasks;

	Mesh* m_pFullScreenQuad = nullptr;



	// ------------------- DXR temp ----------------
	CommandInterface* m_pTempCommandInterface = nullptr;

	// Bottom
	ID3D12Resource1* m_pBottomLevelAS = nullptr;

	// Top
	nv_helpers_dx12::TopLevelASGenerator m_TopLevelAsGenerator;
	AccelerationStructureBuffers m_TopLevelASBuffers;
	std::vector<std::pair<ID3D12Resource1*, DirectX::XMMATRIX>> m_instances;

	AccelerationStructureBuffers CreateBottomLevelAS(std::vector<std::pair<ID3D12Resource1*, uint32_t>> vVertexBuffers);
	void CreateTopLevelAS(std::vector<std::pair<ID3D12Resource1*, DirectX::XMMATRIX>> &instances);
	void CreateAccelerationStructures();
	// ------------------- DXR temp ----------------



	// Group of components that's needed for rendering:
	std::vector<RenderComponent*> m_RenderComponents;

	ViewPool* m_pViewPool = nullptr;
	std::map<LIGHT_TYPE, std::vector<std::tuple<Light*, ConstantBuffer*, ShadowInfo*>>> m_Lights;

	// Current scene to be drawn
	Scene* m_pCurrActiveScene = nullptr;
	CB_PER_SCENE_STRUCT* m_pCbPerSceneData = nullptr;
	ConstantBuffer* m_pCbPerScene = nullptr;

	// update per frame
	CB_PER_FRAME_STRUCT* m_pCbPerFrameData = nullptr;
	ConstantBuffer* m_pCbPerFrame = nullptr;

	// Commandlists holders
	std::vector<ID3D12CommandList*> m_DirectCommandLists[NUM_SWAP_BUFFERS];
	
	// DescriptorHeaps
	std::map<DESCRIPTOR_HEAP_TYPE, DescriptorHeap*> m_DescriptorHeaps = {};

	// Fences
	HANDLE m_EventHandle = nullptr;
	ID3D12Fence1* m_pFenceFrame = nullptr;
	UINT64 m_FenceFrameValue = 0;

	void setRenderTasksPrimaryCamera();
	bool createDevice();
	void createCommandQueues();
	void createSwapChain();
	void createMainDSV();
	void createRootSignature();
	void createFullScreenQuad();
	void initRenderTasks();
	void setRenderTasksRenderComponents();
	void createDescriptorHeaps();
	void createFences();
	void waitForFrame(unsigned int framesToBeAhead = NUM_SWAP_BUFFERS - 1);
	void waitForGPU();

	// Setup the whole scene
	void prepareScene(Scene* activeScene);

	// Submit cbPerSceneData to the copyQueue that updates once
	void submitUploadPerSceneData();
	// Submit cbPerFrameData to the copyQueue that updates each frame
	void submitUploadPerFrameData();

	void toggleFullscreen(WindowChange* event);

	SwapChain* getSwapChain() const;
};

#endif