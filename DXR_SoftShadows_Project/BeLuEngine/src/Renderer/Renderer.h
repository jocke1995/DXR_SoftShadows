#ifndef RENDERER_H
#define RENDERER_H

// Misc
class ThreadPool;
class Window;

#include "DXR_Helpers/DXRHelperrr.h"
#include "D3D12Timer.h"
#include "DX12Tasks/CopyTask.h"
#include "DX12Tasks/RenderTask.h"

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
class RenderTarget;
class Resource;
class ShaderResourceView;
class PingPongResource;

// Enums
enum COMMAND_INTERFACE_TYPE;
enum class DESCRIPTOR_HEAP_TYPE;

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
class Shader;
class ShadowBufferRenderTask;
class TAARenderTask;
class MergeLightningRenderTask;

// Copy
class CopyTask;

// Compute
class ComputeTask;
class GaussianBlurAllShadowsTask;
class FinalBlurAllShadowsTask;

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

struct BLModel
{
	AccelerationStructureBuffers* ASbuffer;
	std::vector<std::pair<ID3D12Resource1*, uint32_t>> vertexBuffers;
	std::vector<std::pair<ID3D12Resource1*, uint32_t>> indexBuffers;
};

struct PointLightRawBufferData
{
	LightHeader header;
	std::vector<PointLight> pls;
};

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
	void InitDXR();

	void UpdateSceneToGPU();

	void SetQuitOnFinish(bool b);
	void SetRTType(std::wstring wstr);
	void SetNumLights(int num);
	void SetSceneName(std::wstring sceneName);
	void SetResultsFileName();
	void OutputTestResults(double dt);

	// Call each frame
	void Update(double dt);
	void SortObjects();
	void ExecuteDXR(double dt);
	void ExecuteInlinePixel(double dt);
	void ExecuteInlineCompute(double dt);
	void ExecuteTEST(double dt);

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
	void submitToCpftAppend(ShaderResource_ContinousMemory* shaderResource_contMem);
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

	// Depthbuffer
	DepthStencil* m_pMainDepthStencil = nullptr;
	ShaderResourceView* m_pDepthBufferSRV = nullptr;

	// Rootsignature
	RootSignature* m_pRootSignature = nullptr;

	// Tasks
	std::vector<ComputeTask*> m_ComputeTasks;
	std::vector<CopyTask*>    m_CopyTasks;
	std::vector<RenderTask*>  m_RenderTasks;

	Mesh* m_pFullScreenQuad = nullptr;

	// ------------------- DXR temp ----------------
	// Test variables
	std::string m_GPUName = "Unknown";
	std::string m_DriverVersion = "Unknown";
	bool m_QuitOnFinish = false;
	std::string m_RTType = "Unknown";
	int m_NumLights = 1;
	std::wstring m_OutputName = L"Results";
	std::wstring m_SceneName = L"none";
	D3D12::D3D12Timer m_DXTimer;



	CommandInterface* m_pTempCommandInterface = nullptr;

	// AS
	AccelerationStructureBuffers m_TopLevelASBuffers;

	// Generators
	nv_helpers_dx12::BottomLevelASGenerator m_BottomLevelASGenerator = {};
	nv_helpers_dx12::TopLevelASGenerator	m_TopLevelAsGenerator = {};

	// All bottomLevel models
	std::vector<BLModel*> m_BottomLevelModels;

	// Objects
	std::vector<std::pair<ID3D12Resource1*, DirectX::XMMATRIX>> m_instances;

	void CreateBottomLevelAS(BLModel** blModel);
	void CreateTopLevelAS(std::vector<std::pair<ID3D12Resource1*, DirectX::XMMATRIX>>& instances);
	void CreateAccelerationStructures();

	// Blur tasks
	GaussianBlurAllShadowsTask* m_GaussianBlurAllShadowsTask;
	void createGaussianBlurTask();

	FinalBlurAllShadowsTask* m_FinalBlurAllShadowsTask;
	void createFinalBlurTask();

	ShadowBufferRenderTask* m_ShadowBufferRenderTask;
	TAARenderTask* m_TAARenderTask = nullptr;
	void createShadowBufferRenderTasks();
	void createTAARenderTasks();
	MergeLightningRenderTask* m_MergeLightningRenderTask;
	void createMergeLightningRenderTasks();
	
	void temporalAccumulation(ID3D12GraphicsCommandList5* cl);
	void spatialAccumulation(ID3D12GraphicsCommandList5* cl);
	void lightningMergeTask(ID3D12GraphicsCommandList5* cl);
	void TAATask(ID3D12GraphicsCommandList5* cl);

	// Gaussian blur each light shadowBuffer
	void GaussianSpatialAccumulation(ID3D12GraphicsCommandList5* cl, unsigned int currentTemporalIndex);

	// Final Blur
	void FinalBlur(ID3D12GraphicsCommandList5* cl);

	ID3D12RootSignature* CreateRayGenSignature();
	ID3D12RootSignature* CreateHitSignature();
	ID3D12RootSignature* CreateMissSignature();
	ID3D12RootSignature* CreateShadowSignature();

	void CreateRaytracingPipeline();

	Shader* m_pRayGenShader = nullptr;
	Shader* m_pHitShader = nullptr;
	Shader* m_pMissShader = nullptr;
	Shader* m_pShadowShader = nullptr;

	ID3D12RootSignature* m_pRayGenSignature = nullptr;
	ID3D12RootSignature* m_pHitSignature = nullptr;
	ID3D12RootSignature* m_pMissSignature = nullptr;
	ID3D12RootSignature* m_pShadowSignature = nullptr;


	// Ray tracing pipeline state
	ID3D12StateObject* m_pRTStateObject = nullptr;
	ID3D12StateObjectProperties* m_pRTStateObjectProps = nullptr;

	// #DXR Resources
	void CreateRaytracingOutputBuffer();
	void CreateShaderResourceHeap();
	Resource* m_pOutputResource = nullptr;

	// DescriptorHeapIndexStart for the AS and outputBuffer
	unsigned int m_DhIndexASOB = 0;
	unsigned int m_DhIndexSoftShadowsUAV = 0;
	unsigned int m_DhIndexSoftShadowsBuffer = 0;

	void CreateShaderBindingTable();
	void CreateSoftShadowLightResources();
	void CreateTAAResource();

	nv_helpers_dx12::ShaderBindingTableGenerator m_SbtHelper;
	ID3D12Resource* m_pSbtStorage;

	// Camera
	DXR_CAMERA* m_pCbCameraData = nullptr;
	ConstantBuffer* m_pCbCamera = nullptr;
	// ------------------- DXR temp ----------------
	PingPongResource* m_pShadowBufferPingPong[MAX_POINT_LIGHTS];
	Resource* m_pShadowBufferResource[MAX_POINT_LIGHTS];
	PingPongResource* m_LightTemporalPingPong[MAX_POINT_LIGHTS][NUM_TEMPORAL_BUFFERS + 1]; // 1 PingPongResource per light, NUM_BUFFERS PingPongResources for temporal accumilation
	Resource* m_LightTemporalResources[MAX_POINT_LIGHTS][NUM_TEMPORAL_BUFFERS + 1];

	Resource* m_pTempInlinePixelUploadResource = nullptr;

	Resource* m_pTAAResource = nullptr;
	PingPongResource* m_pTAAPingPong = nullptr;


	// Group of components that's needed for rendering:
	std::vector<RenderComponent*> m_RenderComponents;

	ViewPool* m_pViewPool = nullptr;

	std::map<LIGHT_TYPE, std::vector<Light*>> m_Lights;

	// ShaderResource will be interpreted as a raw buffer. With a header including common information, and then the lights following.
	std::map<LIGHT_TYPE, ShaderResource_ContinousMemory*> m_LightRawBuffers;
	PointLightRawBufferData plRawBufferData = {};
	void createRawBuffersForLights();

	// Current scene to be drawn
	Scene* m_pCurrActiveScene = nullptr;
	CB_PER_SCENE_STRUCT* m_pCbPerSceneData = nullptr;
	ConstantBuffer* m_pCbPerScene = nullptr;

	// update per frame
	CB_PER_FRAME_STRUCT* m_pCbPerFrameData = nullptr;
	ConstantBuffer* m_pCbPerFrame = nullptr;

	// Commandlists holders
	std::vector<ID3D12CommandList*> m_DirectCommandLists[NUM_SWAP_BUFFERS];

	ID3D12CommandList* m_DXRCpftCommandLists[NUM_SWAP_BUFFERS];
	ID3D12CommandList* m_DXRCodtCommandLists[NUM_SWAP_BUFFERS];
	ID3D12CommandList* m_DepthPrePassCommandLists[NUM_SWAP_BUFFERS];
	ID3D12CommandList* m_GBufferCommandLists[NUM_SWAP_BUFFERS];
	ID3D12CommandList* m_ComputeIRTCommandLists[NUM_SWAP_BUFFERS];

	// G-Buffer renderTargets
	RTV_SRV_RESOURCE m_GBufferNormal = {};

	/* INLINE RT COMPUTETASK data */
	unsigned int m_IRTNumThreadGroupsX = 0;
	unsigned int m_IRTNumThreadGroupsY = 0;
	const unsigned int m_ThreadsPerGroup = 256;

	// DescriptorHeaps
	std::map<DESCRIPTOR_HEAP_TYPE, DescriptorHeap*> m_DescriptorHeaps = {};

	// Fences
	HANDLE m_EventHandle = nullptr;
	ID3D12Fence1* m_pFenceFrame = nullptr;
	UINT64 m_FenceFrameValue = 0;

	void setRenderTasksPrimaryCamera();
	bool createDevice();
	std::string getDriverVersion();
	void createCommandQueues();
	void createSwapChain();
	void createGBufferRenderTargets();
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
