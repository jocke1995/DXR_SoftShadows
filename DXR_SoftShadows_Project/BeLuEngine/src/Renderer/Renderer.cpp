#include "stdafx.h"
#include "Renderer.h"

// Misc
#include "../Misc/AssetLoader.h"
#include "../Misc/Window.h"

// ECS
#include "../ECS/Scene.h"
#include "../ECS/Entity.h"
#include "../ECS/Components/ModelComponent.h"
#include "../ECS/Components/CameraComponent.h"
#include "../ECS/Components/Lights/DirectionalLightComponent.h"
#include "../ECS/Components/Lights/PointLightComponent.h"
#include "../ECS/Components/Lights/SpotLightComponent.h"

// Renderer-Engine 
#include "RootSignature.h"
#include "SwapChain.h"
#include "GPUMemory/DepthStencilView.h"
#include "CommandInterface.h"
#include "DescriptorHeap.h"
#include "Model/Transform.h"
#include "Camera/BaseCamera.h"
#include "Model/Model.h"
#include "Model/Mesh.h"
#include "Texture/Texture.h"
#include "Texture/Texture2D.h"
#include "Texture/Texture2DGUI.h"
#include "Texture/TextureCubeMap.h"
#include "Model/Material.h"
#include "DXILShaderCompiler.h"

#include "RenderView.h"

#include "GPUMemory/GPUMemory.h"

// Graphics
#include "DX12Tasks/RenderTask.h"
#include "DX12Tasks/DepthRenderTask.h"
#include "DX12Tasks/ForwardRenderTask.h"

// Copy 
#include "DX12Tasks/CopyPerFrameTask.h"
#include "DX12Tasks/CopyOnDemandTask.h"

// Compute
#include "DX12Tasks/ComputeTask.h"

// Event
#include "../Events/EventBus.h"

#include "../Misc/CSVExporter.h"

Renderer::Renderer()
{
	EventBus::GetInstance().Subscribe(this, &Renderer::toggleFullscreen);
	m_RenderTasks.resize(RENDER_TASK_TYPE::NR_OF_RENDERTASKS);
	m_CopyTasks.resize(COPY_TASK_TYPE::NR_OF_COPYTASKS);
	m_ComputeTasks.resize(COMPUTE_TASK_TYPE::NR_OF_COMPUTETASKS);
}

Renderer& Renderer::GetInstance()
{
	static Renderer instance;
	return instance;
}

Renderer::~Renderer()
{
	
}

void Renderer::deleteRenderer()
{
	Log::Print("----------------------------  Deleting Renderer  ----------------------------------\n");
	waitForGPU();

	SAFE_RELEASE(&m_pFenceFrame);
	if (!CloseHandle(m_EventHandle))
	{
		Log::Print("Failed To Close Handle... ErrorCode: %d\n", GetLastError());
	}

	SAFE_RELEASE(&m_CommandQueues[COMMAND_INTERFACE_TYPE::DIRECT_TYPE]);
	SAFE_RELEASE(&m_CommandQueues[COMMAND_INTERFACE_TYPE::COMPUTE_TYPE]);
	SAFE_RELEASE(&m_CommandQueues[COMMAND_INTERFACE_TYPE::COPY_TYPE]);

	delete m_pRootSignature;
	delete m_pFullScreenQuad;
	delete m_pSwapChain;
	delete m_pBloomResources;
	delete m_pMainDepthStencil;

	for (auto& pair : m_DescriptorHeaps)
	{
		delete pair.second;
	}

	for (ComputeTask* computeTask : m_ComputeTasks)
		delete computeTask;

	for (CopyTask* copyTask : m_CopyTasks)
		delete copyTask;

	for (RenderTask* renderTask : m_RenderTasks)
		delete renderTask;

	

	delete m_pMousePicker;

	delete m_pViewPool;
	delete m_pCbPerScene;
	delete m_pCbPerSceneData;
	delete m_pCbPerFrame;
	delete m_pCbPerFrameData;
	delete m_pTempCommandInterface;

	// DXR
	
	for (auto i : m_instances)
	{
		SAFE_RELEASE(&i.first);
	}

	m_BottomLevelASBuffers.release();
	m_TopLevelASBuffers.release();
	SAFE_RELEASE(&m_pHitLibrary);
	SAFE_RELEASE(&m_pMissLibrary);
	SAFE_RELEASE(&m_pRayGenLibrary);
	SAFE_RELEASE(&m_pRayGenSignature);
	SAFE_RELEASE(&m_pMissSignature);
	SAFE_RELEASE(&m_pHitSignature);

	SAFE_RELEASE(&m_pRTStateObject);
	SAFE_RELEASE(&m_pRTStateObjectProps);
	



	delete m_pUploadTriVertices;
	SAFE_RELEASE(&m_pOutputResource);
	SAFE_RELEASE(&m_pSbtStorage);

	SAFE_RELEASE(&m_pDevice5);
}

void Renderer::InitD3D12(Window *window, HINSTANCE hInstance, ThreadPool* threadPool)
{
	m_pThreadPool = threadPool;
	m_pWindow = window;

	// Work around for SetStablePowerState to work.
	D3D12EnableExperimentalFeatures(0, nullptr, nullptr, nullptr);

	// Create Device
	if (!createDevice())
	{
		BL_LOG_CRITICAL("Failed to Create Device\n");
	}

	m_pDevice5->SetStablePowerState(true);

	// Create CommandQueues (copy, compute and direct)
	createCommandQueues();

	// Create DescriptorHeaps
	createDescriptorHeaps();

	// Fence for WaitForFrame();
	createFences();

	// Rendertargets
	createSwapChain();

	// Create Main DepthBuffer
	createMainDSV();
	
	// Create Rootsignature
	createRootSignature();

	// DXIL ShaderCompiler
	DXILShaderCompiler::Get()->Init();

	// FullScreenQuad
	createFullScreenQuad();

	// Init Assetloader
	AssetLoader* al = AssetLoader::Get(m_pDevice5, m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV], m_pWindow);

	// Allocate memory for cbPerScene
	m_pCbPerScene = new ConstantBuffer(
		m_pDevice5, 
		sizeof(CB_PER_SCENE_STRUCT),
		L"CB_PER_SCENE",
		m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]
		);
	
	m_pCbPerSceneData = new CB_PER_SCENE_STRUCT();

	// Allocate memory for cbPerFrame
	m_pCbPerFrame = new ConstantBuffer(
		m_pDevice5,
		sizeof(CB_PER_FRAME_STRUCT),
		L"CB_PER_FRAME",
		m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]
	);

	m_pCbPerFrameData = new CB_PER_FRAME_STRUCT();

	// Temp
	m_pTempCommandInterface = new CommandInterface(m_pDevice5, COMMAND_INTERFACE_TYPE::DIRECT_TYPE);
	m_pTempCommandInterface->Reset(0);

	initRenderTasks();



	// RAYTRACING HERE:::::

	createRTTriangle();

	CreateAccelerationStructures();

	CreateRaytracingPipeline();

	// Allocate the buffer storing the raytracing output, with the same dimensions
	// as the target image
	CreateRaytracingOutputBuffer(); // #DXR

	// Create the buffer containing the raytracing result (always output in a
	// UAV), and create the heap referencing the resources used by the raytracing,
	// such as the acceleration structure
	CreateShaderResourceHeap(); // #DXR

	// Create the shader binding table and indicating which shaders
	// are invoked for each instance in the  AS
	CreateShaderBindingTable();

	m_DXTimer.Init(m_pDevice5, 1);
	m_DXTimer.InitGPUFrequency(m_CommandQueues[COMMAND_INTERFACE_TYPE::DIRECT_TYPE]);
	
	submitMeshToCodt(m_pFullScreenQuad);
}

void Renderer::SetQuitOnFinish(bool b)
{
	m_QuitOnFinish = b;
}

void Renderer::SetResultsFileName(std::wstring outputName)
{
	m_OutputName = outputName;
}

void Renderer::Update(double dt)
{
	float3 right = reinterpret_cast<float3&>(m_pScenePrimaryCamera->GetRightVector());
	right.normalize();

	float3 forward = reinterpret_cast<float3&>(m_pScenePrimaryCamera->GetDirection());
	forward.normalize();

	// TODO: fix camera up vector
	float3 up = forward.cross(right);
	up.normalize();

	// Update CB_PER_FRAME data
	m_pCbPerFrameData->camPos = reinterpret_cast<float3&>(m_pScenePrimaryCamera->GetPosition());
	m_pCbPerFrameData->camRight = right;
	m_pCbPerFrameData->camUp = up;
	m_pCbPerFrameData->camForward = forward;
}

void Renderer::SortObjects()
{
	struct DistFromCamera
	{
		double distance;
		RenderComponent* rc;
	};
	
	unsigned int numRenderComponents = m_RenderComponents.size();
	DistFromCamera* distFromCamArr = new DistFromCamera[numRenderComponents];
	
	// Get all the distances of each objects and store them by ID and distance
	DirectX::XMFLOAT3 camPos = m_pScenePrimaryCamera->GetPosition();
	for (unsigned int i = 0; i < numRenderComponents; i++)
	{
		DirectX::XMFLOAT3 objectPos = m_RenderComponents.at(i)->tc->GetTransform()->GetPositionXMFLOAT3();
	
		double distance = sqrt(pow(camPos.x - objectPos.x, 2) +
			pow(camPos.y - objectPos.y, 2) +
			pow(camPos.z - objectPos.z, 2));
	
		// Save the object alongside its distance to the m_pCamera
		distFromCamArr[i].distance = distance;
		distFromCamArr[i].rc = m_RenderComponents.at(i);
	}
	
	// InsertionSort (because its best case is O(N)), 
	// and since this is sorted ((((((EVERY FRAME)))))) this is a good choice of sorting algorithm
	unsigned int j = 0;
	DistFromCamera distFromCamArrTemp = {};
	for (unsigned int i = 1; i < numRenderComponents; i++)
	{
		j = i;
		while (j > 0 && (distFromCamArr[j - 1].distance > distFromCamArr[j].distance))
		{
			// Swap
			distFromCamArrTemp = distFromCamArr[j - 1];
			distFromCamArr[j - 1] = distFromCamArr[j];
			distFromCamArr[j] = distFromCamArrTemp;
			j--;
		}
	}
	
	// Fill the vector with sorted array
	m_RenderComponents.clear();
	for (unsigned int i = 0; i < numRenderComponents; i++)
	{
		m_RenderComponents.push_back(distFromCamArr[i].rc);
	}
	
	// Free memory
	delete[] distFromCamArr;
	
	// Update the entity-arrays inside the rendertasks
	setRenderTasksRenderComponents();
}


void Renderer::Execute()
{
	IDXGISwapChain4* dx12SwapChain = m_pSwapChain->GetDX12SwapChain();
	unsigned int backBufferIndex = dx12SwapChain->GetCurrentBackBufferIndex();
	unsigned int commandInterfaceIndex = m_FrameCounter++ % NUM_SWAP_BUFFERS;

	DX12Task::SetBackBufferIndex(backBufferIndex);
	DX12Task::SetCommandInterfaceIndex(commandInterfaceIndex);

	CopyTask* copyTask = nullptr;
	ComputeTask* computeTask = nullptr;
	RenderTask* renderTask = nullptr;
	/* --------------------- Record command lists --------------------- */

	// TODO: All in one commandlist, or is this ok, since we will only measure the intresting parts anyways...?

	// Copy on demand
	copyTask = m_CopyTasks[COPY_TASK_TYPE::COPY_ON_DEMAND];
	copyTask->Execute();

	// Copy per frame
	copyTask = m_CopyTasks[COPY_TASK_TYPE::COPY_PER_FRAME];
	copyTask->Execute();

	// Depth pre-pass
	renderTask = m_RenderTasks[RENDER_TASK_TYPE::DEPTH_PRE_PASS];
	renderTask->Execute();

	// Opaque draw
	renderTask = m_RenderTasks[RENDER_TASK_TYPE::FORWARD_RENDER];
	renderTask->Execute();

	m_CommandQueues[COMMAND_INTERFACE_TYPE::DIRECT_TYPE]->ExecuteCommandLists(
		m_DirectCommandLists[commandInterfaceIndex].size(),
		m_DirectCommandLists[commandInterfaceIndex].data());

	/* --------------------------------------------------------------- */

	// Wait if the CPU is to far ahead of the gpu
	m_CommandQueues[COMMAND_INTERFACE_TYPE::DIRECT_TYPE]->Signal(m_pFenceFrame, m_FenceFrameValue);
	waitForFrame(0);
	m_FenceFrameValue++;

	/*------------------- Post draw stuff -------------------*/
	// Clear copy on demand
	m_CopyTasks[COPY_TASK_TYPE::COPY_ON_DEMAND]->Clear();

	/*------------------- Present -------------------*/
	HRESULT hr = dx12SwapChain->Present(0, 0);

#ifdef DEBUG
	if (FAILED(hr))
	{
		BL_LOG_CRITICAL("Swapchain Failed to present\n");
	}
#endif
}


struct TestData
{
	std::string GPUcard = "Unknown";
	double nrOfTests = 5;
	bool inlineRT = "False";
};

TestData testData;
CSVExporter csvExporter;

#define DX12TEST(fnc) m_DXTimer.Start(cl, 0);fnc;m_DXTimer.Stop(cl, 0);m_DXTimer.ResolveQueryToCPU(cl, 0);

void Renderer::ExecuteDXR()
{
	IDXGISwapChain4* dx12SwapChain = m_pSwapChain->GetDX12SwapChain();
	unsigned int backBufferIndex = dx12SwapChain->GetCurrentBackBufferIndex();
	unsigned int commandInterfaceIndex = m_FrameCounter++ % NUM_SWAP_BUFFERS;

	m_pTempCommandInterface->Reset(commandInterfaceIndex);
	auto cl = m_pTempCommandInterface->GetCommandList(commandInterfaceIndex);

	const RenderTargetView* swapChainRenderTarget = m_pSwapChain->GetRTV(backBufferIndex);
	ID3D12Resource1* swapChainResource = swapChainRenderTarget->GetResource()->GetID3D12Resource1();

	// Standard inits
	cl->RSSetViewports(1, swapChainRenderTarget->GetRenderView()->GetViewPort());
	cl->RSSetScissorRects(1, swapChainRenderTarget->GetRenderView()->GetScissorRect());
	
	cl->SetComputeRootSignature(m_pRootSignature->GetRootSig());

	// Change state on front/backbuffer
	cl->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		swapChainResource,
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET));

	DescriptorHeap* renderTargetHeap = m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::RTV];
	DescriptorHeap* depthBufferHeap = m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::DSV];

	// RenderTargets
	const unsigned int swapChainIndex = swapChainRenderTarget->GetDescriptorHeapIndex();
	D3D12_CPU_DESCRIPTOR_HANDLE cdhSwapChain = renderTargetHeap->GetCPUHeapAt(swapChainIndex);
	D3D12_CPU_DESCRIPTOR_HANDLE cdhs[] = { cdhSwapChain };

	// Depth
	D3D12_CPU_DESCRIPTOR_HANDLE dsh = depthBufferHeap->GetCPUHeapAt(m_pMainDepthStencil->GetDSV()->GetDescriptorHeapIndex());

	cl->OMSetRenderTargets(1, cdhs, false, &dsh);

	ID3D12DescriptorHeap* dhSRVUAVCBV = m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]->GetID3D12DescriptorHeap();
	cl->SetDescriptorHeaps(1, &dhSRVUAVCBV);

	cl->SetComputeRootDescriptorTable(RS::dtRaytracing, m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]->GetGPUHeapAt(m_DhIndexASOB));
	
	// On the last frame, the raytracing output was used as a copy source, to
	// copy its contents into the render target. Now we need to transition it to
	// a UAV so that the shaders can write in it.
	CD3DX12_RESOURCE_BARRIER transition = CD3DX12_RESOURCE_BARRIER::Transition(
		m_pOutputResource, D3D12_RESOURCE_STATE_COPY_SOURCE,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	cl->ResourceBarrier(1, &transition);


	// Setup the raytracing task
	D3D12_DISPATCH_RAYS_DESC desc = {};
	// The layout of the SBT is as follows: ray generation shader, miss
	// shaders, hit groups. As described in the CreateShaderBindingTable method,
	// all SBT entries of a given type have the same size to allow a fixed stride.

	// The ray generation shaders are always at the beginning of the SBT. 
	uint32_t rayGenerationSectionSizeInBytes = m_SbtHelper.GetRayGenSectionSize();
	desc.RayGenerationShaderRecord.StartAddress = m_pSbtStorage->GetGPUVirtualAddress();
	desc.RayGenerationShaderRecord.SizeInBytes = rayGenerationSectionSizeInBytes;

	


	// The miss shaders are in the second SBT section, right after the ray
	// generation shader. We have one miss shader for the camera rays and one
	// for the shadow rays, so this section has a size of 2*m_sbtEntrySize. We
	// also indicate the stride between the two miss shaders, which is the size
	// of a SBT entry
	uint32_t missSectionSizeInBytes = m_SbtHelper.GetMissSectionSize();
	desc.MissShaderTable.StartAddress = m_pSbtStorage->GetGPUVirtualAddress() + rayGenerationSectionSizeInBytes;
	desc.MissShaderTable.SizeInBytes = missSectionSizeInBytes;
	desc.MissShaderTable.StrideInBytes = m_SbtHelper.GetMissEntrySize();




	// The hit groups section start after the miss shaders. In this sample we
	// have one 1 hit group for the triangle
	uint32_t hitGroupsSectionSize = m_SbtHelper.GetHitGroupSectionSize();
	desc.HitGroupTable.StartAddress = m_pSbtStorage->GetGPUVirtualAddress() +
		rayGenerationSectionSizeInBytes +
		missSectionSizeInBytes;
	desc.HitGroupTable.SizeInBytes = hitGroupsSectionSize;
	desc.HitGroupTable.StrideInBytes = m_SbtHelper.GetHitGroupEntrySize();



	// Dimensions of the image to render, identical to a kernel launch dimension
	desc.Width = m_pWindow->GetScreenWidth();
	desc.Height = m_pWindow->GetScreenHeight();
	desc.Depth = 1;



	// Bind the raytracing pipeline
	cl->SetPipelineState1(m_pRTStateObject);
	// Dispatch the rays and write to the raytracing output

	DX12TEST(cl->DispatchRays(&desc));

	// The raytracing output needs to be copied to the actual render target used
	// for display. For this, we need to transition the raytracing output from a
	// UAV to a copy source, and the render target buffer to a copy destination.
	// We can then do the actual copy, before transitioning the render target
	// buffer into a render target, that will be then used to display the image
	transition = CD3DX12_RESOURCE_BARRIER::Transition(
		m_pOutputResource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_COPY_SOURCE);
	cl->ResourceBarrier(1, &transition);
	transition = CD3DX12_RESOURCE_BARRIER::Transition(
		swapChainRenderTarget->GetResource()->GetID3D12Resource1(), D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_COPY_DEST);
	cl->ResourceBarrier(1, &transition);

	cl->CopyResource(swapChainRenderTarget->GetResource()->GetID3D12Resource1(),
		m_pOutputResource);

	transition = CD3DX12_RESOURCE_BARRIER::Transition(
		swapChainRenderTarget->GetResource()->GetID3D12Resource1(), D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_PRESENT);
	cl->ResourceBarrier(1, &transition);

	cl->Close();

	ID3D12CommandList* cLists[] = {cl};

	m_CommandQueues[COMMAND_INTERFACE_TYPE::DIRECT_TYPE]->ExecuteCommandLists(
		1,
		cLists);

	waitForGPU();

#pragma region TimeMeasurment
	auto timestamps = m_DXTimer.GetTimestampPair(0);
	double dt = (timestamps.Stop - timestamps.Start) * m_DXTimer.GetGPUFreq();
	BL_LOG_INFO("DXR deltaTime: %lf\n", dt);

	static unsigned int nrOfFrames = 0;
	if (nrOfFrames == 0)
	{
		// meta data first row
		csvExporter << testData.GPUcard << "," << testData.nrOfTests << "," << testData.inlineRT <<  "\n";
	}

	if (nrOfFrames < testData.nrOfTests)
	{
		csvExporter << dt << "\n";
	}
	else if (nrOfFrames == testData.nrOfTests)
	{
		csvExporter.Export(m_OutputName);

		BL_LOG_INFO("Exported.......\n");


		// Quit on finish
		if (m_QuitOnFinish)
		{
			PostQuitMessage(0);
		}
	}
	nrOfFrames++;
#pragma endregion TimeMeasurment

	/*------------------- Present -------------------*/
	HRESULT hr = dx12SwapChain->Present(0, 0);

#ifdef DEBUG
	if (FAILED(hr))
	{
		BL_LOG_CRITICAL("Swapchain Failed to present\n");
	}
#endif
}


void Renderer::InitModelComponent(component::ModelComponent* mc)
{
	component::TransformComponent* tc = mc->GetParent()->GetComponent<component::TransformComponent>();

	// Submit to codt
	submitModelToGPU(mc->m_pModel);
	
	// Only add the m_Entities that actually should be drawn
	if (FLAG_DRAW::DRAW_OPAQUE & mc->GetDrawFlag())
	{
		if (tc != nullptr)
		{
			RenderComponent* rc = new RenderComponent();
			rc->mc = mc;
			rc->tc = tc;
			
			// One resource for each mesh
			for (unsigned int i = 0; i < mc->GetNrOfMeshes(); i++)
			{
				rc->CB_PER_OBJECT_UPLOAD_RESOURCES.push_back(new Resource(
					m_pDevice5,
					sizeof(CB_PER_OBJECT_STRUCT),
					RESOURCE_TYPE::UPLOAD,
					L"CB_PER_OBJECT_UPLOAD_RESOURCE_" + std::to_wstring(i) + L"_" + mc->GetModelPath()));	
			}
			// Finally store the object in the in renderer so it can be drawn
			m_RenderComponents.push_back(rc);
		}
	}
}

void Renderer::InitDirectionalLightComponent(component::DirectionalLightComponent* component)
{
	// Assign CBV from the lightPool
	std::wstring resourceName = L"DirectionalLight";
	ConstantBuffer* cb = new ConstantBuffer(m_pDevice5, sizeof(DirectionalLight), resourceName, m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]);

	// Save in m_pRenderer
	m_Lights[LIGHT_TYPE::DIRECTIONAL_LIGHT].push_back(std::make_tuple(component, cb, nullptr));

	
	// Submit to gpu
	CopyTask* copyTask = nullptr;

	if (component->GetLightFlags() & static_cast<unsigned int>(FLAG_LIGHT::STATIC))
	{
		copyTask = m_CopyTasks[COPY_TASK_TYPE::COPY_ON_DEMAND];
	}
	else
	{
		copyTask = m_CopyTasks[COPY_TASK_TYPE::COPY_PER_FRAME];
	}

	const void* data = static_cast<const void*>(component->GetLightData());
	copyTask->Submit(&std::make_tuple(cb->GetUploadResource(), cb->GetDefaultResource(), data));
	
	// We also need to update the indexBuffer with lights if a light is added.
	// The buffer with indices is inside cbPerSceneData, which is updated in the following function:
	submitUploadPerSceneData();
}

void Renderer::InitPointLightComponent(component::PointLightComponent* component)
{
	// Assign CBV from the lightPool
	std::wstring resourceName = L"PointLight";
	ConstantBuffer* cb = new ConstantBuffer(m_pDevice5, sizeof(PointLight), resourceName, m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]);

	// Assign views required for shadows from the lightPool
	ShadowInfo* si = nullptr;

	// Save in m_pRenderer
	m_Lights[LIGHT_TYPE::POINT_LIGHT].push_back(std::make_tuple(component, cb, si));

	// Submit to gpu
	CopyTask* copyTask = nullptr;
	if (component->GetLightFlags() & static_cast<unsigned int>(FLAG_LIGHT::STATIC))
	{
		copyTask = m_CopyTasks[COPY_TASK_TYPE::COPY_ON_DEMAND];
	}
	else
	{
		copyTask = m_CopyTasks[COPY_TASK_TYPE::COPY_PER_FRAME];
	}

	const void* data = static_cast<const void*>(component->GetLightData());
	copyTask->Submit(&std::make_tuple(cb->GetUploadResource(), cb->GetDefaultResource(), data));

	// We also need to update the indexBuffer with lights if a light is added.
	// The buffer with indices is inside cbPerSceneData, which is updated in the following function:
	submitUploadPerSceneData();
}

void Renderer::InitSpotLightComponent(component::SpotLightComponent* component)
{
	// Assign CBV from the lightPool
	std::wstring resourceName = L"SpotLight";
	ConstantBuffer* cb = new ConstantBuffer(m_pDevice5, sizeof(SpotLight), resourceName, m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]);

	// Save in m_pRenderer
	m_Lights[LIGHT_TYPE::SPOT_LIGHT].push_back(std::make_tuple(component, cb, nullptr));

	// Submit to gpu
	CopyTask* copyTask = nullptr;
	if (component->GetLightFlags() & static_cast<unsigned int>(FLAG_LIGHT::STATIC))
	{
		copyTask = m_CopyTasks[COPY_TASK_TYPE::COPY_ON_DEMAND];
	}
	else
	{
		copyTask = m_CopyTasks[COPY_TASK_TYPE::COPY_PER_FRAME];
	}

	const void* data = static_cast<const void*>(component->GetLightData());
	copyTask->Submit(&std::make_tuple(cb->GetUploadResource(), cb->GetDefaultResource(), data));

	// We also need to update the indexBuffer with lights if a light is added.
	// The buffer with indices is inside cbPerSceneData, which is updated in the following function:
	submitUploadPerSceneData();
}

void Renderer::InitCameraComponent(component::CameraComponent* component)
{
	if (component->IsPrimary() == true)
	{
		m_pScenePrimaryCamera = component->GetCamera();
	}
}

void Renderer::UnInitModelComponent(component::ModelComponent* component)
{
	// Remove component from renderComponents
	// TODO: change data structure to allow O(1) add and remove
	for (unsigned int i = 0; i < m_RenderComponents.size(); i++)
	{
		// Remove from renderComponents
		component::ModelComponent* comp = nullptr;
		comp = m_RenderComponents[i]->mc;
		if (comp == component)
		{
			for (unsigned int j = 0; j < comp->GetNrOfMeshes(); j++)
			{
				delete m_RenderComponents[i]->CB_PER_OBJECT_UPLOAD_RESOURCES.at(j);
			}
			
			delete m_RenderComponents[i];
			m_RenderComponents.erase(m_RenderComponents.begin() + i);
			break;
		}
	}

	// Update Render Tasks components (forward the change in renderComponents)
	setRenderTasksRenderComponents();
}

void Renderer::UnInitDirectionalLightComponent(component::DirectionalLightComponent* component)
{
	LIGHT_TYPE type = LIGHT_TYPE::DIRECTIONAL_LIGHT;
	unsigned int count = 0;
	for (auto& tuple : m_Lights[type])
	{
		Light* light = std::get<0>(tuple);

		component::DirectionalLightComponent* dlc = static_cast<component::DirectionalLightComponent*>(light);

		// Remove light if it matches the entity
		if (component == dlc)
		{
			auto& [light, cb, si] = tuple;

			// Free memory so other m_Entities can use it
			delete cb;
			delete si;
			
			// Remove from CopyPerFrame
			CopyPerFrameTask* cpft = nullptr;
			cpft = static_cast<CopyPerFrameTask*>(m_CopyTasks[COPY_TASK_TYPE::COPY_PER_FRAME]);
			cpft->ClearSpecific(cb->GetUploadResource());

			// Finally remove from m_pRenderer
			m_Lights[type].erase(m_Lights[type].begin() + count);

			// Update cbPerScene
			submitUploadPerSceneData();
			break;
		}
		count++;
	}
}

void Renderer::UnInitPointLightComponent(component::PointLightComponent* component)
{
	LIGHT_TYPE type = LIGHT_TYPE::POINT_LIGHT;
	unsigned int count = 0;
	for (auto& tuple : m_Lights[type])
	{
		Light* light = std::get<0>(tuple);

		component::PointLightComponent* plc = static_cast<component::PointLightComponent*>(light);

		// Remove light if it matches the entity
		if (component == plc)
		{
			// Free memory so other m_Entities can use it
			auto& [light, cb, si] = tuple;

			delete cb;

			// Remove from CopyPerFrame
			CopyPerFrameTask* cpft = nullptr;
			cpft = static_cast<CopyPerFrameTask*>(m_CopyTasks[COPY_TASK_TYPE::COPY_PER_FRAME]);
			cpft->ClearSpecific(cb->GetUploadResource());

			// Finally remove from m_pRenderer
			m_Lights[type].erase(m_Lights[type].begin() + count);

			// Update cbPerScene
			submitUploadPerSceneData();
			break;
		}
		count++;
	}
}

void Renderer::UnInitSpotLightComponent(component::SpotLightComponent* component)
{
	LIGHT_TYPE type = LIGHT_TYPE::SPOT_LIGHT;
	unsigned int count = 0;
	for (auto& tuple : m_Lights[type])
	{
		Light* light = std::get<0>(tuple);

		component::SpotLightComponent* slc = static_cast<component::SpotLightComponent*>(light);

		// Remove light if it matches the entity
		if (component == slc)
		{
			// Free memory so other m_Entities can use it
			auto& [light, cb, si] = tuple;

			delete cb;
			delete si;

			// Remove from CopyPerFrame
			CopyPerFrameTask* cpft = nullptr;
			cpft = static_cast<CopyPerFrameTask*>(m_CopyTasks[COPY_TASK_TYPE::COPY_PER_FRAME]);
			cpft->ClearSpecific(cb->GetUploadResource());

			// Finally remove from m_pRenderer
			m_Lights[type].erase(m_Lights[type].begin() + count);

			// Update cbPerScene
			submitUploadPerSceneData();
			break;
		}
		count++;
	}
}

void Renderer::UnInitCameraComponent(component::CameraComponent* component)
{
}

void Renderer::OnResetScene()
{
	m_RenderComponents.clear();
	m_CopyTasks[COPY_TASK_TYPE::COPY_PER_FRAME]->Clear();
	m_pScenePrimaryCamera = nullptr;
}

void Renderer::submitToCodt(std::tuple<Resource*, Resource*, const void*>* Upload_Default_Data)
{
	CopyOnDemandTask* codt = static_cast<CopyOnDemandTask*>(m_CopyTasks[COPY_TASK_TYPE::COPY_ON_DEMAND]);
	codt->Submit(Upload_Default_Data);
}

void Renderer::submitMeshToCodt(Mesh* mesh)
{
	CopyOnDemandTask* codt = static_cast<CopyOnDemandTask*>(m_CopyTasks[COPY_TASK_TYPE::COPY_ON_DEMAND]);

	std::tuple<Resource*, Resource*, const void*> Vert_Upload_Default_Data(mesh->m_pUploadResourceVertices, mesh->m_pDefaultResourceVertices, mesh->m_Vertices.data());
	std::tuple<Resource*, Resource*, const void*> Indi_Upload_Default_Data(mesh->m_pUploadResourceIndices, mesh->m_pDefaultResourceIndices, mesh->m_Indices.data());

	codt->Submit(&Vert_Upload_Default_Data);
	codt->Submit(&Indi_Upload_Default_Data);
}

void Renderer::submitModelToGPU(Model* model)
{
	// Dont submit if already on GPU
	if (AssetLoader::Get()->IsModelLoadedOnGpu(model) == true)
	{
		return;
	}

	for (unsigned int i = 0; i < model->GetSize(); i++)
	{
		Mesh* mesh = model->GetMeshAt(i);

		// Submit Mesh
		submitMeshToCodt(mesh);

		Texture* texture;
		// Submit Material
		texture = model->GetMaterialAt(i)->GetTexture(TEXTURE2D_TYPE::ALBEDO);
		submitTextureToCodt(texture);
		texture = model->GetMaterialAt(i)->GetTexture(TEXTURE2D_TYPE::ROUGHNESS);
		submitTextureToCodt(texture);
		texture = model->GetMaterialAt(i)->GetTexture(TEXTURE2D_TYPE::METALLIC);
		submitTextureToCodt(texture);
		texture = model->GetMaterialAt(i)->GetTexture(TEXTURE2D_TYPE::NORMAL);
		submitTextureToCodt(texture);
		texture = model->GetMaterialAt(i)->GetTexture(TEXTURE2D_TYPE::EMISSIVE);
		submitTextureToCodt(texture);
		texture = model->GetMaterialAt(i)->GetTexture(TEXTURE2D_TYPE::OPACITY);
		submitTextureToCodt(texture);
	}

	AssetLoader::Get()->m_LoadedModels.at(model->GetPath()).first = true;
}

void Renderer::submitTextureToCodt(Texture* texture)
{
	if (AssetLoader::Get()->IsTextureLoadedOnGpu(texture) == true)
	{
		return;
	}

	CopyOnDemandTask* codt = static_cast<CopyOnDemandTask*>(m_CopyTasks[COPY_TASK_TYPE::COPY_ON_DEMAND]);
	codt->SubmitTexture(texture);

	AssetLoader::Get()->m_LoadedTextures.at(texture->GetPath()).first = true;
}

void Renderer::submitToCpft(std::tuple<Resource*, Resource*, const void*>* Upload_Default_Data)
{
	CopyPerFrameTask* cpft = static_cast<CopyPerFrameTask*>(m_CopyTasks[COPY_TASK_TYPE::COPY_PER_FRAME]);
	cpft->Submit(Upload_Default_Data);
}

void Renderer::clearSpecificCpft(Resource* upload)
{
	CopyPerFrameTask* cpft = static_cast<CopyPerFrameTask*>(m_CopyTasks[COPY_TASK_TYPE::COPY_PER_FRAME]);
	cpft->ClearSpecific(upload);
}

DescriptorHeap* Renderer::getCBVSRVUAVdHeap() const
{
	return m_DescriptorHeaps.at(DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV);
}

Scene* const Renderer::GetActiveScene() const
{
	return m_pCurrActiveScene;
}

Window* const Renderer::GetWindow() const
{
	return m_pWindow;
}

struct VertexTemp
{
	DirectX::XMFLOAT3 pos;
};

void Renderer::createRTTriangle()
{
	// Mesh data
	std::vector<VertexTemp> vertexVector;

	VertexTemp vertices[3] = {};
	vertices[0].pos = { 0, 1.0f, 0 };

	vertices[1].pos = { 0.866f,  -0.5f, 0, };

	vertices[2].pos = { -0.866f, -0.5f, 0 };

	for (unsigned int i = 0; i < 4; i++)
	{
		vertexVector.push_back(vertices[i]);
	}


	// create vertices resource
	m_pUploadTriVertices = new Resource(m_pDevice5, vertexVector.size() * sizeof(VertexTemp), RESOURCE_TYPE::UPLOAD, L"TRI_VERTEX_UPLOAD");
	m_pUploadTriVertices->SetData(vertexVector.data());

}

void Renderer::CreateBottomLevelAS(std::vector<std::pair<ID3D12Resource1*, uint32_t>> vVertexBuffers)
{
	// Adding all vertex buffers and not transforming their position.
	for (const auto& buffer : vVertexBuffers)
	{
		m_BottomLevelASGenerator.AddVertexBuffer(buffer.first, 0, buffer.second,
			sizeof(VertexTemp), 0, 0);
	}

	// The AS build requires some scratch space to store temporary information.
	// The amount of scratch memory is dependent on the scene complexity.
	UINT64 scratchSizeInBytes = 0;
	// The final AS also needs to be stored in addition to the existing vertex
	// buffers. It size is also dependent on the scene complexity.
	UINT64 resultSizeInBytes = 0;

	m_BottomLevelASGenerator.ComputeASBufferSizes(m_pDevice5, false, &scratchSizeInBytes,
		&resultSizeInBytes);

	// Scratch
	m_BottomLevelASBuffers.scratch = new Resource(
		m_pDevice5, scratchSizeInBytes,
		RESOURCE_TYPE::DEFAULT, L"scratchBottomLevel",
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	// Result
	m_BottomLevelASBuffers.result = new Resource(
		m_pDevice5, resultSizeInBytes,
		RESOURCE_TYPE::DEFAULT, L"resultBottomLevel",
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);
	
	// Build the acceleration structure. Note that this call integrates a barrier
	// on the generated AS, so that it can be used to compute a top-level AS right
	// after this method.
	m_BottomLevelASGenerator.Generate(m_pTempCommandInterface->GetCommandList(0), m_BottomLevelASBuffers.scratch->GetID3D12Resource1(),
		m_BottomLevelASBuffers.result->GetID3D12Resource1(), false, nullptr);
}

void Renderer::CreateTopLevelAS(std::vector<std::pair<ID3D12Resource1*, DirectX::XMMATRIX>>& instances)
{
	// Gather all the instances into the builder helper
	for (size_t i = 0; i < instances.size(); i++)
	{
		m_TopLevelAsGenerator.AddInstance(instances[i].first,
			instances[i].second, static_cast<unsigned int>(i),
			static_cast<unsigned int>(0));
	}

	// As for the bottom-level AS, the building the AS requires some scratch space
	// to store temporary data in addition to the actual AS. In the case of the
	// top-level AS, the instance descriptors also need to be stored in GPU
	// memory. This call outputs the memory requirements for each (scratch,
	// results, instance descriptors) so that the application can allocate the
	// corresponding memory
	UINT64 scratchSize, resultSize, instanceDescsSize;

	// Prebuild
	m_TopLevelAsGenerator.ComputeASBufferSizes(m_pDevice5, true, &scratchSize, &resultSize, &instanceDescsSize);

	// Create the scratch and result buffers. Since the build is all done on GPU,
	// those can be allocated on the default heap

	// Scratch
	m_TopLevelASBuffers.scratch = new Resource(
		m_pDevice5, scratchSize,
		RESOURCE_TYPE::DEFAULT, L"scratchTopLevel",
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	// Result
	m_TopLevelASBuffers.result = new Resource(
		m_pDevice5, resultSize,
		RESOURCE_TYPE::DEFAULT, L"resultTopLevel",
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);

	// The buffer describing the instances: ID, shader binding information,
	// matrices ... Those will be copied into the buffer by the helper through
	// mapping, so the buffer has to be allocated on the upload heap.

	// Result
	m_TopLevelASBuffers.instanceDesc = new Resource(
		m_pDevice5, instanceDescsSize,
		RESOURCE_TYPE::UPLOAD, L"instanceDescTopLevel",
		D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ);

	// After all the buffers are allocated, or if only an update is required, we
	// can build the acceleration structure. Note that in the case of the update
	// we also pass the existing AS as the 'previous' AS, so that it can be
	// refitted in place.
	m_TopLevelAsGenerator.Generate(m_pTempCommandInterface->GetCommandList(0),
		m_TopLevelASBuffers.scratch->GetID3D12Resource1(),
		m_TopLevelASBuffers.result->GetID3D12Resource1(),
		m_TopLevelASBuffers.instanceDesc->GetID3D12Resource1());
}

void Renderer::CreateAccelerationStructures()
{
	// Build the bottom AS from the Triangle vertex buffer
	ID3D12Resource1* r1 = m_pUploadTriVertices->GetID3D12Resource1();
	unsigned int numVertices = 4;
	CreateBottomLevelAS({ {r1, numVertices} });

	// Just one instance for now
	m_instances = { {m_BottomLevelASBuffers.result->GetID3D12Resource1(), DirectX::XMMatrixIdentity()} };
	CreateTopLevelAS(m_instances);

	// Flush the command list and wait for it to finish
	m_pTempCommandInterface->GetCommandList(0)->Close();
	ID3D12CommandList* ppCommandLists[] = { m_pTempCommandInterface->GetCommandList(0) };
	m_CommandQueues[COMMAND_INTERFACE_TYPE::DIRECT_TYPE]->ExecuteCommandLists(1, ppCommandLists);

	// Wait if the CPU is to far ahead of the gpu
	m_CommandQueues[COMMAND_INTERFACE_TYPE::DIRECT_TYPE]->Signal(m_pFenceFrame, m_FenceFrameValue);
	waitForFrame(0);
	m_FenceFrameValue++;
}

ID3D12RootSignature* Renderer::CreateRayGenSignature()
{
	nv_helpers_dx12::RootSignatureGenerator rsg;

	//rsg.AddHeapRangesParameter(
	//	{ {0 /*u0*/, 1 /*1 descriptor */, 0 /*use the implicit register space 0*/,
	//	  D3D12_DESCRIPTOR_RANGE_TYPE_UAV /* UAV representing the output buffer*/,
	//	  0 /*heap slot where the UAV is defined*/},
	//	 {0 /*t0*/, 1, 0,
	//	  D3D12_DESCRIPTOR_RANGE_TYPE_SRV /*Top-level acceleration structure*/,
	//	  1} });

	return rsg.Generate(m_pDevice5, true);
}

ID3D12RootSignature* Renderer::CreateMissSignature()
{
	//-----------------------------------------------------------------------------
	// The miss shader communicates only through the ray payload, and therefore
	// does not require any resources
	//
	nv_helpers_dx12::RootSignatureGenerator rsc;
	return rsc.Generate(m_pDevice5, true);
}

ID3D12RootSignature* Renderer::CreateHitSignature()
{
	//-----------------------------------------------------------------------------
	// The hit shader communicates only through the ray payload, and therefore does
	// not require any resources
	//
	nv_helpers_dx12::RootSignatureGenerator rsc;
	return rsc.Generate(m_pDevice5, true);
}

void Renderer::CreateRaytracingPipeline()
{
	nv_helpers_dx12::RayTracingPipelineGenerator pipeline(m_pDevice5);

	// The pipeline contains the DXIL code of all the shaders potentially executed
	// during the raytracing process. This section compiles the HLSL code into a
	// set of DXIL libraries. We chose to separate the code in several libraries
	// by semantic (ray generation, hit, miss) for clarity. Any code layout can be
	// used.
	m_pRayGenLibrary = nv_helpers_dx12::CompileShaderLibrary(L"../BeLuEngine/src/Renderer/DXR_Helpers/shaders/RayGen.hlsl");
	m_pMissLibrary = nv_helpers_dx12::CompileShaderLibrary(L"../BeLuEngine/src/Renderer/DXR_Helpers/shaders/Miss.hlsl");
	m_pHitLibrary = nv_helpers_dx12::CompileShaderLibrary(L"../BeLuEngine/src/Renderer/DXR_Helpers/shaders/Hit.hlsl");

	  // In a way similar to DLLs, each library is associated with a number of
	  // exported symbols. This
	  // has to be done explicitly in the lines below. Note that a single library
	  // can contain an arbitrary number of symbols, whose semantic is given in HLSL
	  // using the [shader("xxx")] syntax
	pipeline.AddLibrary(m_pRayGenLibrary, { L"RayGen" });
	pipeline.AddLibrary(m_pMissLibrary, { L"Miss" });
	pipeline.AddLibrary(m_pHitLibrary, { L"ClosestHit" });

	// To be used, each DX12 shader needs a root signature defining which
	// parameters and buffers will be accessed.
	m_pRayGenSignature = CreateRayGenSignature();
	m_pMissSignature = CreateMissSignature();
	m_pHitSignature = CreateHitSignature();

	// 3 different shaders can be invoked to obtain an intersection: an
	// intersection shader is called
	// when hitting the bounding box of non-triangular geometry. This is beyond
	// the scope of this tutorial. An any-hit shader is called on potential
	// intersections. This shader can, for example, perform alpha-testing and
	// discard some intersections. Finally, the closest-hit program is invoked on
	// the intersection point closest to the ray origin. Those 3 shaders are bound
	// together into a hit group.

	// Note that for triangular geometry the intersection shader is built-in. An
	// empty any-hit shader is also defined by default, so in our simple case each
	// hit group contains only the closest hit shader. Note that since the
	// exported symbols are defined above the shaders can be simply referred to by
	// name.

	// Hit group for the triangles, with a shader simply interpolating vertex
	// colors
	pipeline.AddHitGroup(L"HitGroup", L"ClosestHit");

	// The following section associates the root signature to each shader.Note
	// that we can explicitly show that some shaders share the same root signature
	// (eg. Miss and ShadowMiss). Note that the hit shaders are now only referred
	// to as hit groups, meaning that the underlying intersection, any-hit and
	// closest-hit shaders share the same root signature.
	pipeline.AddRootSignatureAssociation(m_pRayGenSignature, { L"RayGen" });
	pipeline.AddRootSignatureAssociation(m_pMissSignature, { L"Miss" });
	pipeline.AddRootSignatureAssociation(m_pHitSignature, { L"HitGroup" });

	// The payload size defines the maximum size of the data carried by the rays,
	// ie. the the data
	// exchanged between shaders, such as the HitInfo structure in the HLSL code.
	// It is important to keep this value as low as possible as a too high value
	// would result in unnecessary memory consumption and cache trashing.
	pipeline.SetMaxPayloadSize(4 * sizeof(float)); // RGB + distance

	// Upon hitting a surface, DXR can provide several attributes to the hit. In
	// our sample we just use the barycentric coordinates defined by the weights
	// u,v of the last two vertices of the triangle. The actual barycentrics can
	// be obtained using float3 barycentrics = float3(1.f-u-v, u, v);
	pipeline.SetMaxAttributeSize(2 * sizeof(float)); // barycentric coordinates

	// The raytracing process can shoot rays from existing hit points, resulting
	// in nested TraceRay calls. Our sample code traces only primary rays, which
	// then requires a trace depth of 1. Note that this recursion depth should be
	// kept to a minimum for best performance. Path tracing algorithms can be
	// easily flattened into a simple loop in the ray generation.
	pipeline.SetMaxRecursionDepth(1);

	// Compile the pipeline for execution on the GPU
	m_pRTStateObject = pipeline.Generate(m_pRootSignature->GetRootSig());

	// Cast the state object into a properties object, allowing to later access
	// the shader pointers by name
	ThrowIfFailed(m_pRTStateObject->QueryInterface(IID_PPV_ARGS(&m_pRTStateObjectProps)));
}

void Renderer::CreateRaytracingOutputBuffer()
{
	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.DepthOrArraySize = 1;
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	// The backbuffer is actually DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, but sRGB
	// formats cannot be used with UAVs. For accuracy we should convert to sRGB
	// ourselves in the shader
	resDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;

	resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	resDesc.Width = m_pWindow->GetScreenWidth();
	resDesc.Height = m_pWindow->GetScreenHeight();
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.MipLevels = 1;
	resDesc.SampleDesc.Count = 1;
	ThrowIfFailed(m_pDevice5->CreateCommittedResource(
		&nv_helpers_dx12::kDefaultHeapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
		D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr,
		IID_PPV_ARGS(&m_pOutputResource)));
}

void Renderer::CreateShaderResourceHeap()
{
	// Create a SRV/UAV/CBV descriptor heap. We need 2 entries - 1 UAV for the
	// raytracing output and 1 SRV for the TLAS

	//m_pSrvUavHeap = new DescriptorHeap(m_pDevice5, DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV);

	// Get a handle to the heap memory on the CPU side, to be able to write the
	// descriptors directly
	m_DhIndexASOB = m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]->GetNextDescriptorHeapIndex(1);
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle =
		m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]->GetCPUHeapAt(m_DhIndexASOB);

	// Create the UAV. Based on the root signature we created it is the first
	// entry. The Create*View methods write the view information directly into
	// srvHandle
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	m_pDevice5->CreateUnorderedAccessView(m_pOutputResource, nullptr, &uavDesc,
		srvHandle);

	// Add the Top Level AS SRV right after the raytracing output buffer
	unsigned int nextDhIndex = m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]->GetNextDescriptorHeapIndex(1);
	srvHandle = m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]->GetCPUHeapAt(nextDhIndex);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.RaytracingAccelerationStructure.Location =
		m_TopLevelASBuffers.result->GetID3D12Resource1()->GetGPUVirtualAddress();
	// Write the acceleration structure view in the heap
	m_pDevice5->CreateShaderResourceView(nullptr, &srvDesc, srvHandle);
}

void Renderer::CreateShaderBindingTable()
{
	// The SBT helper class collects calls to Add*Program.  If called several
	// times, the helper must be emptied before re-adding shaders.
	m_SbtHelper.Reset();

	// The pointer to the beginning of the heap is the only parameter required by
	// shaders without root parameters
	
	// ?? should this be "m_DhIndexASOB" instead of "0"? 
	D3D12_GPU_DESCRIPTOR_HANDLE srvUavHeapHandle = m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]->GetGPUHeapAt(0);

	// The helper treats both root parameter pointers and heap pointers as void*,
	// while DX12 uses the
	// D3D12_GPU_DESCRIPTOR_HANDLE to define heap pointers. The pointer in this
	// struct is a UINT64, which then has to be reinterpreted as a pointer.

	void* heapPointer = reinterpret_cast<void*>(srvUavHeapHandle.ptr);


	// The ray generation only uses heap data
	m_SbtHelper.AddRayGenerationProgram(L"RayGen", { heapPointer });

	// The miss and hit shaders do not access any external resources: instead they
	// communicate their results through the ray payload
	m_SbtHelper.AddMissProgram(L"Miss", {});

	// Adding the triangle hit shader
	m_SbtHelper.AddHitGroup(L"HitGroup", {});



	// Compute the size of the SBT given the number of shaders and their
	// parameters
	uint32_t sbtSize = m_SbtHelper.ComputeSBTSize();

	// Create the SBT on the upload heap. This is required as the helper will use
	// mapping to write the SBT contents. After the SBT compilation it could be
	// copied to the default heap for performance.
	m_pSbtStorage = nv_helpers_dx12::CreateBuffer(
		m_pDevice5, sbtSize, D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ, nv_helpers_dx12::kUploadHeapProps);

	if (!m_pSbtStorage)
	{
		throw std::logic_error("Could not allocate the shader binding table");
	}

	// Compile the SBT from the shader and parameters info
	m_SbtHelper.Generate(m_pSbtStorage, m_pRTStateObjectProps);

}

void Renderer::setRenderTasksPrimaryCamera()
{
	m_RenderTasks[RENDER_TASK_TYPE::DEPTH_PRE_PASS]->SetCamera(m_pScenePrimaryCamera);
	m_RenderTasks[RENDER_TASK_TYPE::FORWARD_RENDER]->SetCamera(m_pScenePrimaryCamera);
}

bool Renderer::createDevice()
{
	bool deviceCreated = false;

#ifdef DEBUG
		//Enable the D3D12 debug layer.
		ID3D12Debug3* debugController = nullptr;
		HMODULE mD3D12 = LoadLibrary(L"D3D12.dll"); // ist�llet f�r GetModuleHandle

		PFN_D3D12_GET_DEBUG_INTERFACE f = (PFN_D3D12_GET_DEBUG_INTERFACE)GetProcAddress(mD3D12, "D3D12GetDebugInterface");
		if (SUCCEEDED(f(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();
			debugController->SetEnableGPUBasedValidation(false);
		}

		IDXGIInfoQueue* dxgiInfoQueue = nullptr;
		unsigned int dxgiFactoryFlags = 0;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue)))) {
			dxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;

			// Break on severity
			dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
			dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);

			// Break on errors
			dxgiInfoQueue->SetBreakOnCategory(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_CATEGORY_UNKNOWN, true);
			dxgiInfoQueue->SetBreakOnCategory(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_CATEGORY_MISCELLANEOUS, true);
			dxgiInfoQueue->SetBreakOnCategory(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_CATEGORY_INITIALIZATION, true);
			dxgiInfoQueue->SetBreakOnCategory(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_CATEGORY_CLEANUP, true);
			dxgiInfoQueue->SetBreakOnCategory(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_CATEGORY_COMPILATION, true);
			dxgiInfoQueue->SetBreakOnCategory(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_CATEGORY_STATE_CREATION, true);
			dxgiInfoQueue->SetBreakOnCategory(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_CATEGORY_STATE_SETTING, true);
			dxgiInfoQueue->SetBreakOnCategory(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_CATEGORY_STATE_GETTING, true);
			dxgiInfoQueue->SetBreakOnCategory(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_CATEGORY_RESOURCE_MANIPULATION, true);
			dxgiInfoQueue->SetBreakOnCategory(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_CATEGORY_EXECUTION, true);
			dxgiInfoQueue->SetBreakOnCategory(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_CATEGORY_SHADER, true);
		}

		SAFE_RELEASE(&debugController);
#endif

	IDXGIFactory6* factory = nullptr;
	IDXGIAdapter1* adapter = nullptr;

	CreateDXGIFactory1(IID_PPV_ARGS(&factory));

	for (unsigned int adapterIndex = 0;; ++adapterIndex)
	{
		adapter = nullptr;
		if (DXGI_ERROR_NOT_FOUND == factory->EnumAdapters1(adapterIndex, &adapter))
		{
			break; // No more adapters
		}
	
		// Check to see if the adapter supports Direct3D 12, but don't create the actual m_pDevice yet.
		ID3D12Device5* pDevice = nullptr;
		HRESULT hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&pDevice));
		
		if (SUCCEEDED(hr))
		{
			DXGI_ADAPTER_DESC adapterDesc = {};
			adapter->GetDesc(&adapterDesc);

			Log::Print("Adapter: %S\n", adapterDesc.Description);
			
			D3D12_FEATURE_DATA_D3D12_OPTIONS5 features5 = {};
			HRESULT hr = pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &features5, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS5));
			SAFE_RELEASE(&pDevice);
			
			if (SUCCEEDED(hr))
			{
				if (features5.RaytracingTier == D3D12_RAYTRACING_TIER_1_1)
				{
					Log::Print("Raytracing tier 1.1 supported!\n");
					break; // found one!
				}
			}
		}
	
		SAFE_RELEASE(&adapter);
	}
	
	if (adapter)
	{
		HRESULT hr = S_OK;
		//Create the actual device.
		if (SUCCEEDED(hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_pDevice5))))
		{
			deviceCreated = true;
		}
		else
		{
			BL_LOG_CRITICAL("Failed to create Device\n");
		}
	
		SAFE_RELEASE(&adapter);
	}
	else
	{
		// Create warpAdapter if no other suitable adapter was found.
		BL_LOG_WARNING("Creating warp-adapter...\n");
		factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter));
		D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_pDevice5));
	}
	SAFE_RELEASE(&factory);

	return deviceCreated;
}

void Renderer::createCommandQueues()
{
	// Direct
	D3D12_COMMAND_QUEUE_DESC cqdDirect = {};
	cqdDirect.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	HRESULT hr;
	hr = m_pDevice5->CreateCommandQueue(&cqdDirect, IID_PPV_ARGS(&m_CommandQueues[COMMAND_INTERFACE_TYPE::DIRECT_TYPE]));
	if (FAILED(hr))
	{
		BL_LOG_CRITICAL("Failed to Create Direct CommandQueue\n");
	}
	m_CommandQueues[COMMAND_INTERFACE_TYPE::DIRECT_TYPE]->SetName(L"DirectQueue");

	// Compute
	D3D12_COMMAND_QUEUE_DESC cqdCompute = {};
	cqdCompute.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
	hr = m_pDevice5->CreateCommandQueue(&cqdCompute, IID_PPV_ARGS(&m_CommandQueues[COMMAND_INTERFACE_TYPE::COMPUTE_TYPE]));
	if (FAILED(hr))
	{
		BL_LOG_CRITICAL("Failed to Create Compute CommandQueue\n");
	}
	m_CommandQueues[COMMAND_INTERFACE_TYPE::COMPUTE_TYPE]->SetName(L"ComputeQueue");

	// Copy
	D3D12_COMMAND_QUEUE_DESC cqdCopy = {};
	cqdCopy.Type = D3D12_COMMAND_LIST_TYPE_COPY;
	hr = m_pDevice5->CreateCommandQueue(&cqdCopy, IID_PPV_ARGS(&m_CommandQueues[COMMAND_INTERFACE_TYPE::COPY_TYPE]));
	if (FAILED(hr))
	{
		BL_LOG_CRITICAL("Failed to Create Copy CommandQueue\n");
	}
	m_CommandQueues[COMMAND_INTERFACE_TYPE::COPY_TYPE]->SetName(L"CopyQueue");
}

void Renderer::createSwapChain()
{
	unsigned int resolutionWidth = m_pWindow->GetScreenWidth();
	unsigned int resolutionHeight = m_pWindow->GetScreenHeight();

	m_pSwapChain = new SwapChain(
		m_pDevice5,
		m_pWindow->GetHwnd(),
		resolutionWidth, resolutionHeight,
		m_CommandQueues[COMMAND_INTERFACE_TYPE::DIRECT_TYPE],
		m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::RTV],
		m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]);
}

void Renderer::createMainDSV()
{
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	unsigned int resolutionWidth = 0;
	unsigned int resolutionHeight = 0;
	HRESULT hr = m_pSwapChain->GetDX12SwapChain()->GetSourceSize(&resolutionWidth, &resolutionHeight);
	if (FAILED(hr))
	{
		BL_LOG_CRITICAL("Failed to GetSourceSize from DX12SwapChain when creating DSV\n");
	}


	m_pMainDepthStencil = new DepthStencil(
		m_pDevice5,
		resolutionWidth, resolutionHeight,
		L"MainDSV",
		&dsvDesc,
		m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::DSV]);
}

void Renderer::createRootSignature()
{
	m_pRootSignature = new RootSignature(m_pDevice5);
}

void Renderer::createFullScreenQuad()
{
	std::vector<Vertex> vertexVector;
	std::vector<unsigned int> indexVector;

	Vertex vertices[4] = {};
	vertices[0].pos = { -1.0f, 1.0f, 1.0f };
	vertices[0].uv = { 0.0f, 0.0f, };

	vertices[1].pos = { -1.0f, -1.0f, 1.0f };
	vertices[1].uv = { 0.0f, 1.0f };

	vertices[2].pos = { 1.0f, 1.0f, 1.0f };
	vertices[2].uv = { 1.0f, 0.0f };

	vertices[3].pos = { 1.0f, -1.0f, 1.0f };
	vertices[3].uv = { 1.0f, 1.0f };

	for (unsigned int i = 0; i < 4; i++)
	{
		vertexVector.push_back(vertices[i]);
	}
	indexVector.push_back(1);
	indexVector.push_back(0);
	indexVector.push_back(3);
	indexVector.push_back(0);
	indexVector.push_back(2);
	indexVector.push_back(3);

	m_pFullScreenQuad = new Mesh(&vertexVector, &indexVector);

	// init dx12 resources
	m_pFullScreenQuad->Init(m_pDevice5, m_DescriptorHeaps.at(DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV));
}

void Renderer::initRenderTasks()
{

#pragma region DepthPrePass

	/* Depth Pre-Pass rendering without stencil testing */
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsdDepthPrePass = {};
	gpsdDepthPrePass.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	// RenderTarget
	gpsdDepthPrePass.NumRenderTargets = 0;
	gpsdDepthPrePass.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
	// Depthstencil usage
	gpsdDepthPrePass.SampleDesc.Count = 1;
	gpsdDepthPrePass.SampleMask = UINT_MAX;
	// Rasterizer behaviour
	gpsdDepthPrePass.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	gpsdDepthPrePass.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	gpsdDepthPrePass.RasterizerState.DepthBias = 0;
	gpsdDepthPrePass.RasterizerState.DepthBiasClamp = 0.0f;
	gpsdDepthPrePass.RasterizerState.SlopeScaledDepthBias = 0.0f;
	gpsdDepthPrePass.RasterizerState.FrontCounterClockwise = false;

	// Specify Blend descriptions
	// copy of defaultRTdesc
	D3D12_RENDER_TARGET_BLEND_DESC depthPrePassRTdesc = {
		false, false,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP, D3D12_COLOR_WRITE_ENABLE_ALL };
	for (unsigned int i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
		gpsdDepthPrePass.BlendState.RenderTarget[i] = depthPrePassRTdesc;

	// Depth descriptor
	D3D12_DEPTH_STENCIL_DESC depthPrePassDsd = {};
	depthPrePassDsd.DepthEnable = true;
	depthPrePassDsd.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depthPrePassDsd.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

	// DepthStencil
	depthPrePassDsd.StencilEnable = false;
	gpsdDepthPrePass.DepthStencilState = depthPrePassDsd;
	gpsdDepthPrePass.DSVFormat = m_pMainDepthStencil->GetDSV()->GetDXGIFormat();

	std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*> gpsdDepthPrePassVector;
	gpsdDepthPrePassVector.push_back(&gpsdDepthPrePass);

	RenderTask* DepthPrePassRenderTask = new DepthRenderTask(
		m_pDevice5,
		m_pRootSignature,
		L"DepthVertex.hlsl", L"DepthPixel.hlsl",
		&gpsdDepthPrePassVector,
		L"DepthPrePassPSO");

	DepthPrePassRenderTask->SetMainDepthStencil(m_pMainDepthStencil);
	DepthPrePassRenderTask->SetSwapChain(m_pSwapChain);
	DepthPrePassRenderTask->SetDescriptorHeaps(m_DescriptorHeaps);

#pragma endregion DepthPrePass

#pragma region ForwardRendering
	/* Forward rendering without stencil testing */
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsdForwardRender = {};
	gpsdForwardRender.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	// RenderTarget
	gpsdForwardRender.NumRenderTargets = 2;
	gpsdForwardRender.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	gpsdForwardRender.RTVFormats[1] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	// Depthstencil usage
	gpsdForwardRender.SampleDesc.Count = 1;
	gpsdForwardRender.SampleMask = UINT_MAX;
	// Rasterizer behaviour
	gpsdForwardRender.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	gpsdForwardRender.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	gpsdForwardRender.RasterizerState.FrontCounterClockwise = false;

	// Specify Blend descriptions
	D3D12_RENDER_TARGET_BLEND_DESC defaultRTdesc = {
		false, false,
		D3D12_BLEND_ZERO, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP, D3D12_COLOR_WRITE_ENABLE_ALL };
	for (unsigned int i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
		gpsdForwardRender.BlendState.RenderTarget[i] = defaultRTdesc;

	// Depth descriptor
	D3D12_DEPTH_STENCIL_DESC dsd = {};
	dsd.DepthEnable = true;
	dsd.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	dsd.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	// DepthStencil
	dsd.StencilEnable = false;
	gpsdForwardRender.DepthStencilState = dsd;
	gpsdForwardRender.DSVFormat = m_pMainDepthStencil->GetDSV()->GetDXGIFormat();

	// All settings are done, now give it to the renderTask
	std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*> gpsdForwardRenderVector;
	gpsdForwardRenderVector.push_back(&gpsdForwardRender);

	RenderTask* forwardRenderTask = new ForwardRenderTask(
		m_pDevice5,
		m_pRootSignature,
		L"ForwardVertex.hlsl", L"ForwardPixel.hlsl",
		&gpsdForwardRenderVector,
		L"ForwardRenderingPSO");

	forwardRenderTask->AddResource("cbPerFrame", m_pCbPerFrame->GetDefaultResource());
	forwardRenderTask->AddResource("cbPerScene", m_pCbPerScene->GetDefaultResource());
	forwardRenderTask->SetMainDepthStencil(m_pMainDepthStencil);
	forwardRenderTask->SetSwapChain(m_pSwapChain);
	forwardRenderTask->SetDescriptorHeaps(m_DescriptorHeaps);

#pragma endregion ForwardRendering

#pragma region ComputeAndCopyTasks
	// CopyTasks
	CopyTask* copyPerFrameTask = new CopyPerFrameTask(m_pDevice5, COMMAND_INTERFACE_TYPE::DIRECT_TYPE);
	CopyTask* copyOnDemandTask = new CopyOnDemandTask(m_pDevice5, COMMAND_INTERFACE_TYPE::DIRECT_TYPE);

#pragma endregion ComputeAndCopyTasks
	// Add the tasks to desired vectors so they can be used in m_pRenderer
	/* -------------------------------------------------------------- */


	/* ------------------------- CopyQueue Tasks ------------------------ */

	m_CopyTasks[COPY_TASK_TYPE::COPY_PER_FRAME] = copyPerFrameTask;
	m_CopyTasks[COPY_TASK_TYPE::COPY_ON_DEMAND] = copyOnDemandTask;

	/* ------------------------- ComputeQueue Tasks ------------------------ */
	
	// Nothing in this project

	/* ------------------------- DirectQueue Tasks ---------------------- */
	m_RenderTasks[RENDER_TASK_TYPE::DEPTH_PRE_PASS] = DepthPrePassRenderTask;
	m_RenderTasks[RENDER_TASK_TYPE::FORWARD_RENDER] = forwardRenderTask;

	// Pushback in the order of execution
	for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		m_DirectCommandLists[i].push_back(copyOnDemandTask->GetCommandInterface()->GetCommandList(i));
	}

	for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		m_DirectCommandLists[i].push_back(copyPerFrameTask->GetCommandInterface()->GetCommandList(i));
	}

	for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		m_DirectCommandLists[i].push_back(DepthPrePassRenderTask->GetCommandInterface()->GetCommandList(i));
	}

	for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		m_DirectCommandLists[i].push_back(forwardRenderTask->GetCommandInterface()->GetCommandList(i));
	}


}

void Renderer::setRenderTasksRenderComponents()
{
	m_RenderTasks[RENDER_TASK_TYPE::DEPTH_PRE_PASS]->SetRenderComponents(&m_RenderComponents);
	m_RenderTasks[RENDER_TASK_TYPE::FORWARD_RENDER]->SetRenderComponents(&m_RenderComponents);
}

void Renderer::createDescriptorHeaps()
{
	m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV] = new DescriptorHeap(m_pDevice5, DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV);
	m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::RTV]		 = new DescriptorHeap(m_pDevice5, DESCRIPTOR_HEAP_TYPE::RTV);
	m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::DSV]		 = new DescriptorHeap(m_pDevice5, DESCRIPTOR_HEAP_TYPE::DSV);
}

void Renderer::createFences()
{
	HRESULT hr = m_pDevice5->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFenceFrame));

	if (FAILED(hr))
	{
		BL_LOG_CRITICAL("Failed to Create Fence\n");
	}
	m_FenceFrameValue = 1;

	// Event handle to use for GPU synchronization
	m_EventHandle = CreateEvent(0, false, false, 0);
}

void Renderer::waitForGPU()
{
	//Signal and increment the fence value.
	const UINT64 oldFenceValue = m_FenceFrameValue;
	m_CommandQueues[COMMAND_INTERFACE_TYPE::DIRECT_TYPE]->Signal(m_pFenceFrame, oldFenceValue);
	m_FenceFrameValue++;

	//Wait until command queue is done.
	if (m_pFenceFrame->GetCompletedValue() < oldFenceValue)
	{
		m_pFenceFrame->SetEventOnCompletion(oldFenceValue, m_EventHandle);
		WaitForSingleObject(m_EventHandle, INFINITE);
	}
}

void Renderer::waitForFrame(unsigned int framesToBeAhead)
{
	static constexpr unsigned int nrOfFenceChangesPerFrame = 1;
	unsigned int fenceValuesToBeAhead = framesToBeAhead * nrOfFenceChangesPerFrame;

	//Wait if the CPU is "framesToBeAhead" number of frames ahead of the GPU
	if (m_pFenceFrame->GetCompletedValue() < m_FenceFrameValue - fenceValuesToBeAhead)
	{
		m_pFenceFrame->SetEventOnCompletion(m_FenceFrameValue - fenceValuesToBeAhead, m_EventHandle);
		WaitForSingleObject(m_EventHandle, INFINITE);
	}
}

void Renderer::prepareScene(Scene* activeScene)
{
	submitUploadPerFrameData();
	submitUploadPerSceneData();

	// -------------------- DEBUG STUFF --------------------
	// Test to change m_pCamera to the shadow casting m_lights cameras
	//auto& tuple = m_Lights[LIGHT_TYPE::DIRECTIONAL_LIGHT].at(0);
	//BaseCamera* tempCam = std::get<0>(tuple)->GetCamera();
	//m_pScenePrimaryCamera = tempCam;

	if (m_pScenePrimaryCamera == nullptr)
	{
		BL_LOG_CRITICAL("No primary camera was set in scenes\n");

		// Todo: Set default m_pCamera
	}

	setRenderTasksRenderComponents();
	setRenderTasksPrimaryCamera();
}

void Renderer::submitUploadPerSceneData()
{
	*m_pCbPerSceneData = {};
	// ----- directional lights -----
	m_pCbPerSceneData->Num_Dir_Lights = static_cast<unsigned int>(m_Lights[LIGHT_TYPE::DIRECTIONAL_LIGHT].size());
	unsigned int index = 0;
	for (auto& tuple : m_Lights[LIGHT_TYPE::DIRECTIONAL_LIGHT])
	{
		m_pCbPerSceneData->dirLightIndices[index].x = static_cast<float>(std::get<1>(tuple)->GetCBV()->GetDescriptorHeapIndex());
		index++;
	}
	// ----- directional m_lights -----

	// ----- point lights -----
	m_pCbPerSceneData->Num_Point_Lights = static_cast<unsigned int>(m_Lights[LIGHT_TYPE::POINT_LIGHT].size());
	index = 0;
	for (auto& tuple : m_Lights[LIGHT_TYPE::POINT_LIGHT])
	{
		m_pCbPerSceneData->pointLightIndices[index].x = static_cast<float>(std::get<1>(tuple)->GetCBV()->GetDescriptorHeapIndex());
		index++;
	}
	// ----- point m_lights -----

	// ----- spot lights -----
	m_pCbPerSceneData->Num_Spot_Lights = static_cast<unsigned int>(m_Lights[LIGHT_TYPE::SPOT_LIGHT].size());
	index = 0;
	for (auto& tuple : m_Lights[LIGHT_TYPE::SPOT_LIGHT])
	{
		m_pCbPerSceneData->spotLightIndices[index].x = static_cast<float>(std::get<1>(tuple)->GetCBV()->GetDescriptorHeapIndex());
		index++;
	}
	// ----- spot m_lights -----
	
	// Submit CB_PER_SCENE to be uploaded to VRAM
	CopyOnDemandTask* codt = static_cast<CopyOnDemandTask*>(m_CopyTasks[COPY_TASK_TYPE::COPY_ON_DEMAND]);
	const void* data = static_cast<const void*>(m_pCbPerSceneData);
	codt->Submit(&std::make_tuple(m_pCbPerScene->GetUploadResource(), m_pCbPerScene->GetDefaultResource(), data));
}

void Renderer::submitUploadPerFrameData()
{
	// Submit dynamic-light-data to be uploaded to VRAM
	CopyPerFrameTask* cpft = static_cast<CopyPerFrameTask*>(m_CopyTasks[COPY_TASK_TYPE::COPY_PER_FRAME]);

	// CB_PER_FRAME_STRUCT
	if (cpft != nullptr)
	{
		const void* data = static_cast<void*>(m_pCbPerFrameData);
		cpft->Submit(&std::tuple(m_pCbPerFrame->GetUploadResource(), m_pCbPerFrame->GetDefaultResource(), data));
	}
}

void Renderer::toggleFullscreen(WindowChange* event)
{
	m_FenceFrameValue++;
	m_CommandQueues[COMMAND_INTERFACE_TYPE::DIRECT_TYPE]->Signal(m_pFenceFrame, m_FenceFrameValue);

	// Wait for all frames
	waitForFrame(0);

	// Wait for the threads which records the commandlists to complete
	//m_pThreadPool->WaitForThreads(FLAG_THREAD::RENDER);

	for (auto task : m_RenderTasks)
	{
		for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
		{
			task->GetCommandInterface()->Reset(i);
		}
	}
	for (auto task : m_CopyTasks)
	{
		for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
		{
			task->GetCommandInterface()->Reset(i);
		}
	}
	for (auto task : m_ComputeTasks)
	{
		for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
		{
			task->GetCommandInterface()->Reset(i);
		}
	}

	m_pSwapChain->ToggleWindowMode(m_pDevice5,
		m_pWindow->GetHwnd(),
		m_CommandQueues[COMMAND_INTERFACE_TYPE::DIRECT_TYPE],
		m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::RTV],
		m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]);

	// Change the member variables of the window class to match the swapchain
	UINT width = 0, height = 0;
	if (m_pSwapChain->IsFullscreen())
	{
		m_pSwapChain->GetDX12SwapChain()->GetSourceSize(&width, &height);
	}
	else
	{
		// Earlier it read from options. now just set to 800/600
		width = 800;
		height = 600;
	}

	Window* window = const_cast<Window*>(m_pWindow);
	window->SetScreenWidth(width);
	window->SetScreenHeight(height);

	for (auto task : m_RenderTasks)
	{
		for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
		{
			task->GetCommandInterface()->GetCommandList(i)->Close();
		}
	}
	for (auto task : m_CopyTasks)
	{
		for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
		{
			task->GetCommandInterface()->GetCommandList(i)->Close();
		}
	}
	for (auto task : m_ComputeTasks)
	{
		for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
		{
			task->GetCommandInterface()->GetCommandList(i)->Close();
		}
	}
}

SwapChain* Renderer::getSwapChain() const
{
	return m_pSwapChain;
}
