#include "stdafx.h"
#include "Renderer.h"

UINT resolutionWidth = 2560;
UINT resolutionHeight = 1440;

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
#include "RenderView.h"
#include "PipelineState/GraphicsState.h"

#include "DXILShaderCompiler.h"
#include "Shader.h"

#include "GPUMemory/GPUMemory.h"
#include "GPUMemory/PingPongResource.h"

// Graphics
#include "DX12Tasks/RenderTask.h"
#include "DX12Tasks/DepthRenderTask.h"
#include "DX12Tasks/ShadowBufferRenderTask.h"
#include "DX12Tasks/TAARenderTask.h"
#include "DX12Tasks/ForwardRenderTask.h"
#include "DX12Tasks/GBufferRenderTask.h"
#include "DX12Tasks/MergeLightningRenderTask.h"

// Copy 
#include "DX12Tasks/CopyPerFrameTask.h"
#include "DX12Tasks/CopyOnDemandTask.h"

// Compute
#include "DX12Tasks/InlineRTComputeTask.h"
#include "DX12Tasks/GaussianBlurAllShadowsTask.h"
#include "DX12Tasks/FinalBlurAllShadowsTask.h"

// Event
#include "../Events/EventBus.h"

#include "../Misc/CSVExporter.h"


#define FRAMES_TO_MEASURE 60*60 // 1*60*60
double resultAverage0 = -1;
double resultAverage1 = -1;
double CPUresultAverage = -1;
CSVExporter csvExporter;
CSVExporter csvExporter2;
std::vector<double> frameData0;
std::vector<double> frameData1;
std::vector<double> CPUframeData;

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
	BL_LOG("----------------------------  Deleting Renderer  ----------------------------------\n");
	waitForGPU();

	SAFE_RELEASE(&m_pFenceFrame);
	if (!CloseHandle(m_EventHandle))
	{
		BL_LOG_WARNING("Failed To Close Handle... ErrorCode: %d\n", GetLastError());
	}

	SAFE_RELEASE(&m_CommandQueues[COMMAND_INTERFACE_TYPE::DIRECT_TYPE]);
	SAFE_RELEASE(&m_CommandQueues[COMMAND_INTERFACE_TYPE::COMPUTE_TYPE]);
	SAFE_RELEASE(&m_CommandQueues[COMMAND_INTERFACE_TYPE::COPY_TYPE]);

	delete m_pRootSignature;
	delete m_pFullScreenQuad;
	delete m_pSwapChain;
	delete m_pMainDepthStencil;
	delete m_pDepthBufferSRV;

	// GBuffers
	delete m_GBufferNormal.resource;
	delete m_GBufferNormal.srv;
	delete m_GBufferNormal.rtv;

	delete m_pTAAPingPong;
	delete m_pTAAResource;

	for (unsigned int i = 0; i < MAX_POINT_LIGHTS; i++)
	{
		delete m_pShadowBufferPingPong[i];
		delete m_pShadowBufferResource[i];
	}

	for (int z = 0; z < NUM_TEMPORAL_BUFFERS + 1; z++)
	{
		for (unsigned int i = 0; i < MAX_POINT_LIGHTS; i++)
		{
			delete m_LightTemporalPingPong[i][z];
			delete m_LightTemporalResources[i][z];
		}
	}

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

	delete m_pViewPool;
	delete m_pCbPerScene;
	delete m_pCbPerSceneData;
	delete m_pCbPerFrame;
	delete m_pCbPerFrameData;
	delete m_pTempCommandInterface;

	// ----------------------- DXR ----------------------- 
	
	for (auto i : m_instances)
	{
		SAFE_RELEASE(&i.first);
	}

	m_TopLevelASBuffers.~AccelerationStructureBuffers();

	delete m_pRayGenShader;
	delete m_pHitShader;
	delete m_pMissShader;
	delete m_pShadowShader;

	delete m_pCbCamera;
	delete m_pCbCameraData;

	// TODO: DELETE all light types
	delete m_LightRawBuffers[LIGHT_TYPE::POINT_LIGHT]->shaderResource;
	delete m_LightRawBuffers[LIGHT_TYPE::POINT_LIGHT];
	//delete m_Lights[LIGHT_TYPE::DIRECTIONAL_LIGHT].first;
	//delete m_Lights[LIGHT_TYPE::SPOT_LIGHT].first;

	SAFE_RELEASE(&m_pRayGenSignature);
	SAFE_RELEASE(&m_pHitSignature);
	SAFE_RELEASE(&m_pMissSignature);
	SAFE_RELEASE(&m_pShadowSignature);

	SAFE_RELEASE(&m_pRTStateObject);
	SAFE_RELEASE(&m_pRTStateObjectProps);
	
	delete m_pOutputResource;
	delete m_pTempInlinePixelUploadResource;
	

	delete m_ShadowBufferRenderTask;
	delete m_GaussianBlurAllShadowsTask;
	delete m_FinalBlurAllShadowsTask;
	delete m_TAARenderTask;
	delete m_MergeLightningRenderTask;

	SAFE_RELEASE(&m_pSbtStorage);

	SAFE_RELEASE(&m_pDevice5);

	for (BLModel* blModel : m_BottomLevelModels)
	{
		blModel->ASbuffer->release();
		delete blModel->ASbuffer;
		delete blModel;
	}
	m_BottomLevelModels.clear();

	m_TopLevelASBuffers.release();
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

	// GBuffer
	createGBufferRenderTargets();

	// Create Main DepthBuffer
	createMainDSV();
	
	CreateTAAResource();

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
	*m_pCbPerSceneData = {};

	// Allocate memory for cbPerFrame
	m_pCbPerFrame = new ConstantBuffer(
		m_pDevice5,
		sizeof(CB_PER_FRAME_STRUCT),
		L"CB_PER_FRAME",
		m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]
	);

	m_pCbPerFrameData = new CB_PER_FRAME_STRUCT();


	// Temp
	frameData0.reserve(FRAMES_TO_MEASURE*2);
	frameData1.reserve(FRAMES_TO_MEASURE*2);
	CPUframeData.reserve(FRAMES_TO_MEASURE*2);

	// Pushback dummy value for first frame
	CPUframeData.push_back(-1);

	m_pTempCommandInterface = new CommandInterface(m_pDevice5, COMMAND_INTERFACE_TYPE::DIRECT_TYPE);
	m_pTempCommandInterface->Reset(0);

	// Resources for softshadows in blurTask/ShadowBufferTask
	CreateSoftShadowLightResources();

	createShadowBufferRenderTasks();
	createGaussianBlurTask();
	createFinalBlurTask();
	createMergeLightningRenderTasks();

	initRenderTasks();

	submitMeshToCodt(m_pFullScreenQuad);

	createRawBuffersForLights();

	// DXR
	m_pCbCamera = new ConstantBuffer(m_pDevice5, sizeof(DXR_CAMERA), L"DXR_CAMERA", m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]);
	m_pCbCameraData = new DXR_CAMERA();
}

void Renderer::createGaussianBlurTask()
{
	// ComputeTasks
	std::vector<std::pair<std::wstring, std::wstring>> csNamePSOName;
	csNamePSOName.push_back(std::make_pair(L"GaussianBlurHorizontal.hlsl", L"GaussianblurHorizontalPSO"));
	csNamePSOName.push_back(std::make_pair(L"GaussianBlurVertical.hlsl", L"GaussianblurVerticalPSO"));

	m_GaussianBlurAllShadowsTask = new GaussianBlurAllShadowsTask(
			m_pDevice5, m_pRootSignature,
			m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV],
			csNamePSOName,
			COMMAND_INTERFACE_TYPE::DIRECT_TYPE,
			resolutionWidth, resolutionHeight,
			FLAG_THREAD::RENDER);

	m_GaussianBlurAllShadowsTask->SetDescriptorHeaps(m_DescriptorHeaps);
	m_GaussianBlurAllShadowsTask->SetCommandInterface(m_pTempCommandInterface);
}

void Renderer::createFinalBlurTask()
{
	// ComputeTasks
	std::vector<std::pair<std::wstring, std::wstring>> csNamePSOName;
	csNamePSOName.push_back(std::make_pair(L"FinalBlurHorizontal.hlsl", L"FinalblurHorizontalPSO"));
	csNamePSOName.push_back(std::make_pair(L"FinalBlurVertical.hlsl"  , L"FinalblurVerticalPSO"));

	m_FinalBlurAllShadowsTask = new FinalBlurAllShadowsTask(
		m_pDevice5, m_pRootSignature,
		m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV],
		csNamePSOName,
		COMMAND_INTERFACE_TYPE::DIRECT_TYPE,
		resolutionWidth, resolutionHeight,
		FLAG_THREAD::RENDER);

	m_FinalBlurAllShadowsTask->SetDescriptorHeaps(m_DescriptorHeaps);
	m_FinalBlurAllShadowsTask->SetCommandInterface(m_pTempCommandInterface);
	m_FinalBlurAllShadowsTask->AddResource("cbPerScene", m_pCbPerScene->GetDefaultResource());
}

void Renderer::createShadowBufferRenderTasks()
{
	/* Depth Pre-Pass rendering without stencil testing */
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsdShadowBufferPass = {};
	gpsdShadowBufferPass.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	// RenderTarget
	gpsdShadowBufferPass.NumRenderTargets = 0;
	gpsdShadowBufferPass.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
	// Depthstencil usage
	gpsdShadowBufferPass.SampleDesc.Count = 1;
	gpsdShadowBufferPass.SampleMask = UINT_MAX;
	// Rasterizer behaviour
	gpsdShadowBufferPass.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	gpsdShadowBufferPass.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	gpsdShadowBufferPass.RasterizerState.DepthBias = 0;
	gpsdShadowBufferPass.RasterizerState.DepthBiasClamp = 0.0f;
	gpsdShadowBufferPass.RasterizerState.SlopeScaledDepthBias = 0.0f;
	gpsdShadowBufferPass.RasterizerState.FrontCounterClockwise = false;
	gpsdShadowBufferPass.RasterizerState.ForcedSampleCount = 1;

	// Specify Blend descriptions
	// copy of defaultRTdesc
	D3D12_RENDER_TARGET_BLEND_DESC shadowBufferPassRTdesc = {
		false, false,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP, D3D12_COLOR_WRITE_ENABLE_ALL };
	for (unsigned int i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
		gpsdShadowBufferPass.BlendState.RenderTarget[i] = shadowBufferPassRTdesc;

	// Depth descriptor

	// DepthStencil
	gpsdShadowBufferPass.DepthStencilState = {};
	gpsdShadowBufferPass.DepthStencilState.DepthEnable = false;

	std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*> gpsdShadowBufferPassVector;
	gpsdShadowBufferPassVector.push_back(&gpsdShadowBufferPass);

	m_ShadowBufferRenderTask = new ShadowBufferRenderTask(
		m_pDevice5,
		m_pRootSignature,
		L"ShadowBufferVertex.hlsl", L"ShadowBufferPixel.hlsl",
		&gpsdShadowBufferPassVector,
		L"ShadowBufferPassPSO");

	m_ShadowBufferRenderTask->SetMainDepthStencil(m_pMainDepthStencil);
	m_ShadowBufferRenderTask->SetSwapChain(m_pSwapChain);
	m_ShadowBufferRenderTask->SetFullScreenQuadMesh(m_pFullScreenQuad);
	m_ShadowBufferRenderTask->SetDescriptorHeaps(m_DescriptorHeaps);
	m_ShadowBufferRenderTask->SetCommandInterface(m_pTempCommandInterface);
	m_ShadowBufferRenderTask->CreateSlotInfo();
}

void Renderer::createTAARenderTasks()
{
	/* Depth Pre-Pass rendering without stencil testing */
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsdTAAPass = {};
	gpsdTAAPass.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	// RenderTarget
	gpsdTAAPass.NumRenderTargets = 0;
	gpsdTAAPass.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
	// Depthstencil usage
	gpsdTAAPass.SampleDesc.Count = 1;
	gpsdTAAPass.SampleMask = UINT_MAX;
	// Rasterizer behaviour
	gpsdTAAPass.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	gpsdTAAPass.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	gpsdTAAPass.RasterizerState.DepthBias = 0;
	gpsdTAAPass.RasterizerState.DepthBiasClamp = 0.0f;
	gpsdTAAPass.RasterizerState.SlopeScaledDepthBias = 0.0f;
	gpsdTAAPass.RasterizerState.FrontCounterClockwise = false;
	gpsdTAAPass.RasterizerState.ForcedSampleCount = 1;

	// Specify Blend descriptions
	// copy of defaultRTdesc
	D3D12_RENDER_TARGET_BLEND_DESC blendTAARTdesc = {
		false, false,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP, D3D12_COLOR_WRITE_ENABLE_ALL };
	for (unsigned int i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
		gpsdTAAPass.BlendState.RenderTarget[i] = blendTAARTdesc;

	// Depth descriptor

	// DepthStencil
	gpsdTAAPass.DepthStencilState = {};
	gpsdTAAPass.DepthStencilState.DepthEnable = false;

	std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*> gpsdTAAPassVector;
	gpsdTAAPassVector.push_back(&gpsdTAAPass);

	m_TAARenderTask = new TAARenderTask(
		m_pDevice5,
		m_pRootSignature,
		L"TAAVertex.hlsl", L"TAAPixel.hlsl",
		&gpsdTAAPassVector,
		L"TAAPassPSO");

	m_TAARenderTask->SetMainDepthStencil(m_pMainDepthStencil);
	m_TAARenderTask->SetSwapChain(m_pSwapChain);
	m_TAARenderTask->SetFullScreenQuadMesh(m_pFullScreenQuad);
	m_TAARenderTask->SetDescriptorHeaps(m_DescriptorHeaps);
	m_TAARenderTask->SetCommandInterface(m_pTempCommandInterface);
	m_TAARenderTask->CreateSlotInfo();
	m_TAARenderTask->SetTAAPingPongResource(m_pTAAPingPong);
	m_TAARenderTask->SetCurrFrameUAVIndex(m_DhIndexASOB);
}

void Renderer::createMergeLightningRenderTasks()
{
	/* Depth Pre-Pass rendering without stencil testing */
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsd = {};
	gpsd.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	// RenderTarget
	gpsd.NumRenderTargets = 0;
	gpsd.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
	// Depthstencil usage
	gpsd.SampleDesc.Count = 1;
	gpsd.SampleMask = UINT_MAX;
	// Rasterizer behaviour
	gpsd.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	gpsd.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	gpsd.RasterizerState.DepthBias = 0;
	gpsd.RasterizerState.DepthBiasClamp = 0.0f;
	gpsd.RasterizerState.SlopeScaledDepthBias = 0.0f;
	gpsd.RasterizerState.FrontCounterClockwise = false;

	// Specify Blend descriptions
	// copy of defaultRTdesc
	D3D12_RENDER_TARGET_BLEND_DESC RTBlenddesc = {
		false, false,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP, D3D12_COLOR_WRITE_ENABLE_ALL };
	for (unsigned int i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
		gpsd.BlendState.RenderTarget[i] = RTBlenddesc;

	// Depth descriptor

	// DepthStencil
	gpsd.DepthStencilState = {};
	gpsd.DepthStencilState.DepthEnable = false;

	std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*> gpsdVector;
	gpsdVector.push_back(&gpsd);

	m_MergeLightningRenderTask = new MergeLightningRenderTask(
		m_pDevice5,
		m_pRootSignature,
		L"MergeLightningVertex.hlsl", L"MergeLightningPixel.hlsl",
		&gpsdVector,
		L"MergePassPSO");

	m_MergeLightningRenderTask->SetMainDepthStencil(m_pMainDepthStencil);
	m_MergeLightningRenderTask->SetSwapChain(m_pSwapChain);
	m_MergeLightningRenderTask->SetFullScreenQuadMesh(m_pFullScreenQuad);
	m_MergeLightningRenderTask->SetDescriptorHeaps(m_DescriptorHeaps);
	m_MergeLightningRenderTask->SetCommandInterface(m_pTempCommandInterface);
	m_MergeLightningRenderTask->CreateSlotInfo();

	m_MergeLightningRenderTask->AddResource("GBufferNormal", m_GBufferNormal.resource);
}

void Renderer::InitDXR()
{
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
	m_DXTimer.Init(m_pDevice5, 2);
	m_DXTimer.InitGPUFrequency(m_CommandQueues[COMMAND_INTERFACE_TYPE::DIRECT_TYPE]);

	// Uses m_DhIndexASOB
	createTAARenderTasks();
}

void Renderer::UpdateSceneToGPU()
{
	DX12Task::SetCommandInterfaceIndex(0);

	// Copy on demand
	m_CopyTasks[COPY_TASK_TYPE::COPY_ON_DEMAND]->Execute();

	m_CommandQueues[COMMAND_INTERFACE_TYPE::DIRECT_TYPE]->ExecuteCommandLists(1, &m_DXRCodtCommandLists[0]);

	/*------------------- Post draw stuff -------------------*/
	waitForGPU();

}

void Renderer::SetQuitOnFinish(bool b)
{
	m_QuitOnFinish = b;
}

void Renderer::SetRTType(std::wstring wstr)
{
	m_RTType = to_string(wstr);
}

void Renderer::SetNumLights(int num)
{
	m_NumLights = num;
}

void Renderer::SetSceneName(std::wstring sceneName)
{
	m_SceneName = sceneName;
}

void Renderer::SetResultsFileName()
{
	// Not used
	m_OutputName = to_wstring("../" + m_GPUName + "_" + m_RTType + "_" + to_string(m_SceneName));
}

void Renderer::OutputTestResults(double dt)
{
	// !!!!NOTE THAT CPU FRAME TIME IS 1 FRAME BEHIND!!!!

	static int framesMeasured = 0;
	const int framesToSkip = 500;

	const int totalFramesToMeasure = FRAMES_TO_MEASURE + framesToSkip + 1; // +1 since we skip the first frames´ value

	// Skip the first frames
	if (m_FrameCounter <= framesToSkip)
	{
		framesMeasured++;
		return;
	}
	else if (framesMeasured < totalFramesToMeasure)
	{
		auto timestamps0 = m_DXTimer.GetTimestampPair(0);
		auto timestamps1 = m_DXTimer.GetTimestampPair(1);
		double dtMS0 = (timestamps0.Stop - timestamps0.Start) * m_DXTimer.GetGPUFreq();
		double dtMS1 = (timestamps1.Stop - timestamps1.Start) * m_DXTimer.GetGPUFreq();
		framesMeasured++;

		// Save frame time
		frameData0.push_back(dtMS0);
		frameData1.push_back(dtMS1);
		CPUframeData.push_back(dt * 1000);
	}
	else if (framesMeasured >= totalFramesToMeasure)
	{
		std::wstring name = to_wstring("../" + to_string(m_SceneName) + "_" + std::to_string(m_NumLights) + "_" + std::to_string(FRAMES_TO_MEASURE) + "_" + m_RTType);
		std::wstring outputName = name + L".csv";
		std::wstring outputName2 = name + L"_" + to_wstring(m_GPUName) + L"_" + to_wstring(m_DriverVersion) + L".csv";

		// Compute average
		double sum0 = 0;
		double sum1 = 0;
		double CPUsum = 0;
		for (unsigned int i = 0; i < frameData0.size(); i++)
		{
			// Skip first frame since cpuframedata has a dummy value
			if (i == 0)
				continue;

			sum0 += frameData0.at(i);
			sum1 += frameData1.at(i);
			CPUsum += CPUframeData.at(i);
		}

		resultAverage0 = sum0 / (double)FRAMES_TO_MEASURE;
		resultAverage1 = sum1 / (double)FRAMES_TO_MEASURE;
		CPUresultAverage = CPUsum / (double)FRAMES_TO_MEASURE;

		// Header
		if (csvExporter.IsFileEmpty(outputName))
		{
			csvExporter << std::string("GPU Name") << "," << "Driver version" << "," << "DispatchRays Time (ms)" << "," << "DispatchRays Time (ms)++" << "," << "Frame Time (ms)" << "\n";
		}

		csvExporter << m_GPUName << "," << m_DriverVersion << "," << resultAverage0 << "," << resultAverage1 << "," << CPUresultAverage << "\n";


		// Header
		if (csvExporter2.IsFileEmpty(outputName2))
		{
			csvExporter2 << std::string("DispatchRays Time (ms)") << "," << "DispatchRays Time (ms)++" << "," << "Frame Time (ms)" << "\n";
		}

		// Fill all frames data
		for (unsigned int i = 0; i < frameData0.size(); i++)
		{
			if (i == 0)
				continue;
			csvExporter2 << frameData0.at(i) << "," << frameData1.at(i) << "," << CPUframeData.at(i) << "\n";
		}
		
		csvExporter.Append(outputName);
		csvExporter2.Append(outputName2);

		BL_LOG_INFO("Exported.......\n");


		// Quit on finish
		if (m_QuitOnFinish)
		{
			PostQuitMessage(0);
		}
		
		// Results will not be accurate since exporting takes performance
		// Anyway, set test to start // Currently set to not test again
		framesMeasured = -10000000000;
	}
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
	m_pCbPerFrameData->frameCounter = m_FrameCounter;

	// DXR cam
	m_pCbCameraData->projection  = *m_pScenePrimaryCamera->GetProjMatrix();
	m_pCbCameraData->projectionI = *m_pScenePrimaryCamera->GetProjMatrixInverse();
	m_pCbCameraData->view		 = *m_pScenePrimaryCamera->GetViewMatrix();
	m_pCbCameraData->viewI		 = *m_pScenePrimaryCamera->GetViewMatrixInverse();
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

#define DX12TEST(code, x) m_DXTimer.Start(cl,##x);##code;m_DXTimer.Stop(cl,##x);m_DXTimer.ResolveQueryToCPU(cl,##x);

void Renderer::ExecuteDXR(double dt)
{
	m_FrameCounter = m_FrameCounter + 1;

	IDXGISwapChain4* dx12SwapChain = m_pSwapChain->GetDX12SwapChain();
	unsigned int backBufferIndex = dx12SwapChain->GetCurrentBackBufferIndex();
	
	unsigned int commandInterfaceIndex = m_FrameCounter % NUM_SWAP_BUFFERS;
	unsigned int currentLightTemporalBuffer = m_FrameCounter % (NUM_TEMPORAL_BUFFERS + 1);

	const RenderTargetView* swapChainRenderTarget = m_pSwapChain->GetRTV(backBufferIndex);
	ID3D12DescriptorHeap* dhCBVSRVUAV = m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]->GetID3D12DescriptorHeap();

	/* ------------------------------------- COPY DATA NOT MEASURED ------------------------------------- */
	DX12Task::SetCommandInterfaceIndex(commandInterfaceIndex);

	// Copy per frame
	m_CopyTasks[COPY_TASK_TYPE::COPY_PER_FRAME]->Execute();
	m_CommandQueues[COMMAND_INTERFACE_TYPE::DIRECT_TYPE]->ExecuteCommandLists(
		1,
		&m_DXRCpftCommandLists[commandInterfaceIndex]);
	/* ------------------------------------- COPY DATA NOT MEASURED ------------------------------------- */

	m_pTempCommandInterface->Reset(0);
	auto cl = m_pTempCommandInterface->GetCommandList(0);

	m_DXTimer.Start(cl, 1);

	// Depth pre-pass
	m_RenderTasks[RENDER_TASK_TYPE::DEPTH_PRE_PASS]->Execute();

	// GBuffer (Normals only atm)
	m_RenderTasks[RENDER_TASK_TYPE::GBUFFER_PASS]->Execute();

	CD3DX12_RESOURCE_BARRIER transition;

#pragma region RayTrace
	m_DXTimer.Start(cl, 0);
	cl->SetComputeRootSignature(m_pRootSignature->GetRootSig());
	cl->SetDescriptorHeaps(1, &dhCBVSRVUAV);
	cl->SetComputeRootDescriptorTable(RS::dtRaytracing, m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]->GetGPUHeapAt(m_DhIndexASOB));
	cl->SetComputeRootDescriptorTable(RS::dtSRV, m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]->GetGPUHeapAt(0));
	unsigned int softShadowHeapOffset = m_DhIndexSoftShadowsUAV + 2 * MAX_POINT_LIGHTS * currentLightTemporalBuffer;
	cl->SetComputeRootDescriptorTable(RS::dtUAV, m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]->GetGPUHeapAt(softShadowHeapOffset));
	cl->SetComputeRootConstantBufferView(RS::CB_PER_FRAME, m_pCbPerFrame->GetDefaultResource()->GetGPUVirtualAdress());
	cl->SetComputeRootConstantBufferView(RS::CB_PER_SCENE, m_pCbPerScene->GetDefaultResource()->GetGPUVirtualAdress());
	cl->SetComputeRootConstantBufferView(RS::CBV0, m_pCbCamera->GetDefaultResource()->GetGPUVirtualAdress());
	cl->SetComputeRootShaderResourceView(RS::SRV0, m_LightRawBuffers[LIGHT_TYPE::POINT_LIGHT]->shaderResource->GetUploadResource()->GetGPUVirtualAdress());

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


	// The hit groups section start after the miss shaders.
	uint32_t hitGroupsSectionSize = m_SbtHelper.GetHitGroupSectionSize();
	desc.HitGroupTable.StartAddress = m_pSbtStorage->GetGPUVirtualAddress() +
		rayGenerationSectionSizeInBytes +
		missSectionSizeInBytes;
	desc.HitGroupTable.SizeInBytes = hitGroupsSectionSize;
	desc.HitGroupTable.StrideInBytes = m_SbtHelper.GetHitGroupEntrySize();


	// Dimensions of the image to render, identical to a kernel launch dimension
	desc.Width = resolutionWidth;// m_pWindow->GetScreenWidth();
	desc.Height = resolutionHeight;// m_pWindow->GetScreenHeight();
	desc.Depth = 1;

	// Bind the raytracing pipeline
	cl->SetPipelineState1(m_pRTStateObject);
	// Dispatch the rays and write to the raytracing output


	// Set temporal buffers written to UAV
	for (unsigned int i = 0; i < m_Lights[LIGHT_TYPE::POINT_LIGHT].size(); i++)
	{
		cl->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
			m_LightTemporalResources[i][currentLightTemporalBuffer]->GetID3D12Resource1(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	}

	cl->DispatchRays(&desc);
	
	m_DXTimer.Stop(cl, 0);
#pragma endregion RayTrace

	
	// Execute BlurTasks, output to m_LightTemporalResources
	GaussianSpatialAccumulation(cl, currentLightTemporalBuffer);

	
	// Execute ShadowBufferTask, output to m_ShadowBuffer
	temporalAccumulation(cl);

	FinalBlur(cl);

	// Calculate Light and output to m_Output
	lightningMergeTask(cl);
	// TAA
	TAATask(cl);

	m_DXTimer.Stop(cl, 1);




	transition = CD3DX12_RESOURCE_BARRIER::Transition(
		m_pOutputResource->GetID3D12Resource1(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, // StateBefore
		D3D12_RESOURCE_STATE_COPY_SOURCE);	   // StateAfter
	cl->ResourceBarrier(1, &transition);
	
	
	transition = CD3DX12_RESOURCE_BARRIER::Transition(
		swapChainRenderTarget->GetResource()->GetID3D12Resource1(),
		D3D12_RESOURCE_STATE_PRESENT,	 // StateBefore
		D3D12_RESOURCE_STATE_COPY_DEST); // StateAfter
	cl->ResourceBarrier(1, &transition);
	
	
	cl->CopyResource(
		swapChainRenderTarget->GetResource()->GetID3D12Resource1(),	// Dest
		m_pOutputResource->GetID3D12Resource1());					// Source
	
	
	transition = CD3DX12_RESOURCE_BARRIER::Transition(
		swapChainRenderTarget->GetResource()->GetID3D12Resource1(),
		D3D12_RESOURCE_STATE_COPY_DEST, // StateBefore
		D3D12_RESOURCE_STATE_PRESENT);	// StateAfter
	cl->ResourceBarrier(1, &transition);
	
	transition = CD3DX12_RESOURCE_BARRIER::Transition(
		m_pOutputResource->GetID3D12Resource1(),
		D3D12_RESOURCE_STATE_COPY_SOURCE,		// StateBefore
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS);	// StateAfter
	cl->ResourceBarrier(1, &transition);

	m_DXTimer.ResolveQueryToCPU(cl, 0);
	m_DXTimer.ResolveQueryToCPU(cl, 1);

	cl->Close();
	ID3D12CommandList* cLists[] = { cl };
	m_CommandQueues[COMMAND_INTERFACE_TYPE::DIRECT_TYPE]->ExecuteCommandLists(1, cLists);
	/*------------------- Post draw stuff -------------------*/
	waitForGPU();

	OutputTestResults(dt);

	/*------------------- Present -------------------*/
	HRESULT hr = dx12SwapChain->Present(0, 0);

#ifdef DEBUG
	if (FAILED(hr))
	{
		BL_LOG_CRITICAL("Swapchain Failed to present\n");
	}
#endif
}

void Renderer::ExecuteInlinePixel(double dt)
{
	m_FrameCounter = m_FrameCounter + 1;

	IDXGISwapChain4* dx12SwapChain = m_pSwapChain->GetDX12SwapChain();
	unsigned int backBufferIndex = dx12SwapChain->GetCurrentBackBufferIndex();

	unsigned int commandInterfaceIndex = m_FrameCounter % NUM_SWAP_BUFFERS;
	unsigned int currentLightTemporalBuffer = m_FrameCounter % (NUM_TEMPORAL_BUFFERS + 1);

	const RenderTargetView* swapChainRenderTarget = m_pSwapChain->GetRTV(backBufferIndex);
	ID3D12DescriptorHeap* dhCBVSRVUAV = m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]->GetID3D12DescriptorHeap();

	/* ------------------------------------- COPY DATA NOT MEASURED ------------------------------------- */
	DX12Task::SetCommandInterfaceIndex(commandInterfaceIndex);

	// Copy per frame
	m_CopyTasks[COPY_TASK_TYPE::COPY_PER_FRAME]->Execute();
	m_CommandQueues[COMMAND_INTERFACE_TYPE::DIRECT_TYPE]->ExecuteCommandLists(
		1,
		&m_DXRCpftCommandLists[commandInterfaceIndex]);
	/* ------------------------------------- COPY DATA NOT MEASURED ------------------------------------- */

	m_pTempCommandInterface->Reset(0);
	auto cl = m_pTempCommandInterface->GetCommandList(0);

	m_DXTimer.Start(cl, 1);

	// Depth pre-pass
	m_RenderTasks[RENDER_TASK_TYPE::DEPTH_PRE_PASS]->Execute();

	// GBuffer (Normals only atm)
	m_RenderTasks[RENDER_TASK_TYPE::GBUFFER_PASS]->Execute();

	CD3DX12_RESOURCE_BARRIER transition;


#pragma region InlineRTPixel
	m_DXTimer.Start(cl, 0);
	cl->SetGraphicsRootSignature(m_pRootSignature->GetRootSig());

	ID3D12DescriptorHeap * d3d12DescriptorHeap = m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]->GetID3D12DescriptorHeap();
	cl->SetDescriptorHeaps(1, &d3d12DescriptorHeap);

	cl->SetGraphicsRootDescriptorTable(RS::dtRaytracing, m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]->GetGPUHeapAt(m_DhIndexASOB));
	cl->SetGraphicsRootDescriptorTable(RS::dtSRV, m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]->GetGPUHeapAt(0));
	unsigned int softShadowHeapOffset = m_DhIndexSoftShadowsUAV + 2 * MAX_POINT_LIGHTS * currentLightTemporalBuffer;
	cl->SetGraphicsRootDescriptorTable(RS::dtUAV, m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]->GetGPUHeapAt(softShadowHeapOffset));

	cl->SetGraphicsRootConstantBufferView(RS::CB_PER_FRAME, m_pCbPerFrame->GetDefaultResource()->GetGPUVirtualAdress());
	cl->SetGraphicsRootConstantBufferView(RS::CB_PER_SCENE, m_pCbPerScene->GetDefaultResource()->GetGPUVirtualAdress());
	cl->SetGraphicsRootConstantBufferView(RS::CBV0, m_pCbCamera->GetDefaultResource()->GetGPUVirtualAdress());
	cl->SetGraphicsRootShaderResourceView(RS::SRV0, m_LightRawBuffers[LIGHT_TYPE::POINT_LIGHT]->shaderResource->GetUploadResource()->GetGPUVirtualAdress());

	DescriptorHeap* depthBufferHeap = m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::DSV];

	// Depth
	D3D12_CPU_DESCRIPTOR_HANDLE dsh = depthBufferHeap->GetCPUHeapAt(m_pMainDepthStencil->GetDSV()->GetDescriptorHeapIndex());
	cl->OMSetRenderTargets(0, nullptr, true, &dsh);

	const D3D12_VIEWPORT viewPortSwapChain = *swapChainRenderTarget->GetRenderView()->GetViewPort();
	const D3D12_RECT rectSwapChain = *swapChainRenderTarget->GetRenderView()->GetScissorRect();

	const D3D12_RECT * rect = swapChainRenderTarget->GetRenderView()->GetScissorRect();
	cl->RSSetViewports(1, &viewPortSwapChain);
	cl->RSSetScissorRects(1, &rectSwapChain);
	cl->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Draw for every Rendercomponent with stencil testing disabled
	ID3D12PipelineState* pipelineState = m_RenderTasks[RENDER_TASK_TYPE::FORWARD_RENDER]->GetPipelineState(0)->GetPSO();
	cl->SetPipelineState(pipelineState);

	// Set temporal buffers written to UAV
	for (unsigned int i = 0; i < m_Lights[LIGHT_TYPE::POINT_LIGHT].size(); i++)
	{
		cl->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
			m_LightTemporalResources[i][currentLightTemporalBuffer]->GetID3D12Resource1(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	}

	// Create a CB_PER_OBJECT struct
	SlotInfo slotInfo = {};
	slotInfo.vertexDataIndex = m_pFullScreenQuad->m_pVertexBufferSRV->GetDescriptorHeapIndex();

	float4x4 mat = {};
	CB_PER_OBJECT_STRUCT perObject = { mat, mat, slotInfo };

	// Temp, should not SetData here
	m_pTempInlinePixelUploadResource->SetData(&perObject);
	cl->SetGraphicsRootConstantBufferView(RS::CB_PER_OBJECT_CBV, m_pTempInlinePixelUploadResource->GetGPUVirtualAdress());

	cl->IASetIndexBuffer(m_pFullScreenQuad->GetIndexBufferView());

	transition = CD3DX12_RESOURCE_BARRIER::Transition(
		m_pMainDepthStencil->GetDefaultResource()->GetID3D12Resource1(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,		// StateBefore
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);	// StateAfter

	cl->DrawIndexedInstanced(6, 1, 0, 0, 0); // Draw fullscreenquad

	transition = CD3DX12_RESOURCE_BARRIER::Transition(
		m_pMainDepthStencil->GetDefaultResource()->GetID3D12Resource1(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,		// StateBefore
		D3D12_RESOURCE_STATE_DEPTH_WRITE);	// StateAfter

	m_DXTimer.Stop(cl, 0);

#pragma endregion InlineRTPixel
	
	// Execute BlurTasks, output to m_LightTemporalResources
	GaussianSpatialAccumulation(cl, currentLightTemporalBuffer);

	// Execute ShadowBufferTask, output to m_ShadowBuffer
	temporalAccumulation(cl);

	FinalBlur(cl);

	// Calculate Light and output to m_Output
	lightningMergeTask(cl);

	// TAA
	TAATask(cl);
	m_DXTimer.Stop(cl, 1);



	transition = CD3DX12_RESOURCE_BARRIER::Transition(
		m_pOutputResource->GetID3D12Resource1(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, // StateBefore
		D3D12_RESOURCE_STATE_COPY_SOURCE);	   // StateAfter
	cl->ResourceBarrier(1, &transition);


	transition = CD3DX12_RESOURCE_BARRIER::Transition(
		swapChainRenderTarget->GetResource()->GetID3D12Resource1(),
		D3D12_RESOURCE_STATE_PRESENT,	 // StateBefore
		D3D12_RESOURCE_STATE_COPY_DEST); // StateAfter
	cl->ResourceBarrier(1, &transition);


	cl->CopyResource(
		swapChainRenderTarget->GetResource()->GetID3D12Resource1(),	// Dest
		m_pOutputResource->GetID3D12Resource1());					// Source


	transition = CD3DX12_RESOURCE_BARRIER::Transition(
		swapChainRenderTarget->GetResource()->GetID3D12Resource1(),
		D3D12_RESOURCE_STATE_COPY_DEST, // StateBefore
		D3D12_RESOURCE_STATE_PRESENT);	// StateAfter
	cl->ResourceBarrier(1, &transition);

	transition = CD3DX12_RESOURCE_BARRIER::Transition(
		m_pOutputResource->GetID3D12Resource1(),
		D3D12_RESOURCE_STATE_COPY_SOURCE,		// StateBefore
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS);	// StateAfter
	cl->ResourceBarrier(1, &transition);

	m_DXTimer.ResolveQueryToCPU(cl, 0);
	m_DXTimer.ResolveQueryToCPU(cl, 1);

	cl->Close();
	ID3D12CommandList* cLists[] = { cl };
	m_CommandQueues[COMMAND_INTERFACE_TYPE::DIRECT_TYPE]->ExecuteCommandLists(1, cLists);
	/*------------------- Post draw stuff -------------------*/
	waitForGPU();

	OutputTestResults(dt);

	/*------------------- Present -------------------*/
	HRESULT hr = dx12SwapChain->Present(0, 0);

#ifdef DEBUG
	if (FAILED(hr))
	{
		BL_LOG_CRITICAL("Swapchain Failed to present\n");
	}
#endif
}

void Renderer::ExecuteInlineCompute(double dt)
{
	m_FrameCounter = m_FrameCounter + 1;

	IDXGISwapChain4* dx12SwapChain = m_pSwapChain->GetDX12SwapChain();
	unsigned int backBufferIndex = dx12SwapChain->GetCurrentBackBufferIndex();

	unsigned int commandInterfaceIndex = m_FrameCounter % NUM_SWAP_BUFFERS;
	unsigned int currentLightTemporalBuffer = m_FrameCounter % (NUM_TEMPORAL_BUFFERS + 1);

	const RenderTargetView* swapChainRenderTarget = m_pSwapChain->GetRTV(backBufferIndex);
	ID3D12DescriptorHeap* dhCBVSRVUAV = m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]->GetID3D12DescriptorHeap();

	/* ------------------------------------- COPY DATA NOT MEASURED ------------------------------------- */
	DX12Task::SetCommandInterfaceIndex(commandInterfaceIndex);

	// Copy per frame
	m_CopyTasks[COPY_TASK_TYPE::COPY_PER_FRAME]->Execute();
	m_CommandQueues[COMMAND_INTERFACE_TYPE::DIRECT_TYPE]->ExecuteCommandLists(
		1,
		&m_DXRCpftCommandLists[commandInterfaceIndex]);
	/* ------------------------------------- COPY DATA NOT MEASURED ------------------------------------- */

	m_pTempCommandInterface->Reset(0);
	auto cl = m_pTempCommandInterface->GetCommandList(0);

	m_DXTimer.Start(cl, 1);

	// Depth pre-pass
	m_RenderTasks[RENDER_TASK_TYPE::DEPTH_PRE_PASS]->Execute();

	// GBuffer (Normals only atm)
	m_RenderTasks[RENDER_TASK_TYPE::GBUFFER_PASS]->Execute();

	CD3DX12_RESOURCE_BARRIER transition;

	
#pragma region InlineRTCompute
	m_DXTimer.Start(cl, 0);

	cl->SetComputeRootSignature(m_pRootSignature->GetRootSig());

	cl->SetDescriptorHeaps(1, &dhCBVSRVUAV);

	cl->SetComputeRootDescriptorTable(RS::dtRaytracing, m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]->GetGPUHeapAt(m_DhIndexASOB));
	cl->SetComputeRootDescriptorTable(RS::dtSRV, m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]->GetGPUHeapAt(0));
	unsigned int softShadowHeapOffset = m_DhIndexSoftShadowsUAV + 2 * MAX_POINT_LIGHTS * currentLightTemporalBuffer;
	cl->SetComputeRootDescriptorTable(RS::dtUAV, m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]->GetGPUHeapAt(softShadowHeapOffset));

	cl->SetComputeRootConstantBufferView(RS::CB_PER_FRAME, m_pCbPerFrame->GetDefaultResource()->GetGPUVirtualAdress());
	cl->SetComputeRootConstantBufferView(RS::CB_PER_SCENE, m_pCbPerScene->GetDefaultResource()->GetGPUVirtualAdress());
	cl->SetComputeRootConstantBufferView(RS::CBV0, m_pCbCamera->GetDefaultResource()->GetGPUVirtualAdress());
	cl->SetComputeRootShaderResourceView(RS::SRV0, m_LightRawBuffers[LIGHT_TYPE::POINT_LIGHT]->shaderResource->GetUploadResource()->GetGPUVirtualAdress());


	// Draw for every Rendercomponent with stencil testing disabled
	ID3D12PipelineState* pipelineState = m_ComputeTasks[COMPUTE_TASK_TYPE::INLINE_RT]->GetPipelineState(0)->GetPSO();
	cl->SetPipelineState(pipelineState);


	// Set temporal buffers written to UAV
	for (unsigned int i = 0; i < m_Lights[LIGHT_TYPE::POINT_LIGHT].size(); i++)
	{
		cl->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
			m_LightTemporalResources[i][currentLightTemporalBuffer]->GetID3D12Resource1(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	}

	cl->Dispatch(m_IRTNumThreadGroupsX, m_IRTNumThreadGroupsY, 1);

	m_DXTimer.Stop(cl, 0);

#pragma endregion InlineRTCompute

	// Execute BlurTasks, output to m_LightTemporalResources
	GaussianSpatialAccumulation(cl, currentLightTemporalBuffer);

	// Execute ShadowBufferTask, output to m_ShadowBuffer
	temporalAccumulation(cl);

	FinalBlur(cl);

	// Calculate Light and output to m_Output
	lightningMergeTask(cl);

	// TAA
	TAATask(cl);
	m_DXTimer.Stop(cl, 1);


	transition = CD3DX12_RESOURCE_BARRIER::Transition(
		m_pOutputResource->GetID3D12Resource1(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, // StateBefore
		D3D12_RESOURCE_STATE_COPY_SOURCE);	   // StateAfter
	cl->ResourceBarrier(1, &transition);


	transition = CD3DX12_RESOURCE_BARRIER::Transition(
		swapChainRenderTarget->GetResource()->GetID3D12Resource1(),
		D3D12_RESOURCE_STATE_PRESENT,	 // StateBefore
		D3D12_RESOURCE_STATE_COPY_DEST); // StateAfter
	cl->ResourceBarrier(1, &transition);

	cl->CopyResource(
		swapChainRenderTarget->GetResource()->GetID3D12Resource1(),	// Dest
		m_pOutputResource->GetID3D12Resource1());											// Source

	transition = CD3DX12_RESOURCE_BARRIER::Transition(
		swapChainRenderTarget->GetResource()->GetID3D12Resource1(),
		D3D12_RESOURCE_STATE_COPY_DEST, // StateBefore
		D3D12_RESOURCE_STATE_PRESENT);	// StateAfter
	cl->ResourceBarrier(1, &transition);

	transition = CD3DX12_RESOURCE_BARRIER::Transition(
		m_pOutputResource->GetID3D12Resource1(),
		D3D12_RESOURCE_STATE_COPY_SOURCE,		// StateBefore
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS);	// StateAfter
	cl->ResourceBarrier(1, &transition);


	m_DXTimer.ResolveQueryToCPU(cl, 0);
	m_DXTimer.ResolveQueryToCPU(cl, 1);

	cl->Close();
	ID3D12CommandList* cLists[] = { cl };
	m_CommandQueues[COMMAND_INTERFACE_TYPE::DIRECT_TYPE]->ExecuteCommandLists(1, cLists);

	/*------------------- Post draw stuff -------------------*/
	waitForGPU();

	OutputTestResults(dt);

	/*------------------- Present -------------------*/
	HRESULT hr = dx12SwapChain->Present(0, 0);

#ifdef DEBUG
	if (FAILED(hr))
	{
		BL_LOG_CRITICAL("Swapchain Failed to present\n");
	}
#endif
}

void Renderer::ExecuteTEST(double dt)
{
	m_FrameCounter = m_FrameCounter + 1;

	IDXGISwapChain4* dx12SwapChain = m_pSwapChain->GetDX12SwapChain();
	unsigned int backBufferIndex = dx12SwapChain->GetCurrentBackBufferIndex();

	unsigned int commandInterfaceIndex = m_FrameCounter % NUM_SWAP_BUFFERS;
	unsigned int currentLightTemporalBuffer = m_FrameCounter % (NUM_TEMPORAL_BUFFERS + 1);

	const RenderTargetView* swapChainRenderTarget = m_pSwapChain->GetRTV(backBufferIndex);
	ID3D12DescriptorHeap* dhCBVSRVUAV = m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]->GetID3D12DescriptorHeap();

	/* ------------------------------------- COPY DATA ------------------------------------- */
	DX12Task::SetCommandInterfaceIndex(commandInterfaceIndex);

	// Copy per frame
	m_CopyTasks[COPY_TASK_TYPE::COPY_PER_FRAME]->Execute();
	m_CommandQueues[COMMAND_INTERFACE_TYPE::DIRECT_TYPE]->ExecuteCommandLists(
		1,
		&m_DXRCpftCommandLists[commandInterfaceIndex]);

	// Depth pre-pass
	m_RenderTasks[RENDER_TASK_TYPE::DEPTH_PRE_PASS]->Execute();
	m_CommandQueues[COMMAND_INTERFACE_TYPE::DIRECT_TYPE]->ExecuteCommandLists(
		1,
		&m_DepthPrePassCommandLists[commandInterfaceIndex]);

	// GBuffer (Normals only atm)
	m_RenderTasks[RENDER_TASK_TYPE::GBUFFER_PASS]->Execute();
	m_CommandQueues[COMMAND_INTERFACE_TYPE::DIRECT_TYPE]->ExecuteCommandLists(
		1,
		&m_GBufferCommandLists[commandInterfaceIndex]);
	/* ------------------------------------- COPY DATA ------------------------------------- */

	m_pTempCommandInterface->Reset(0);
	auto cl = m_pTempCommandInterface->GetCommandList(0);

	CD3DX12_RESOURCE_BARRIER transition;

	m_DXTimer.Start(cl, 1);
#pragma region RayTrace
	DX12TEST(

	cl->SetComputeRootSignature(m_pRootSignature->GetRootSig());
	cl->SetDescriptorHeaps(1, &dhCBVSRVUAV);
	cl->SetComputeRootDescriptorTable(RS::dtRaytracing, m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]->GetGPUHeapAt(m_DhIndexASOB));
	cl->SetComputeRootDescriptorTable(RS::dtSRV, m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]->GetGPUHeapAt(0));
	unsigned int softShadowHeapOffset = m_DhIndexSoftShadowsUAV + 2 * MAX_POINT_LIGHTS * currentLightTemporalBuffer;
	cl->SetComputeRootDescriptorTable(RS::dtUAV, m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]->GetGPUHeapAt(softShadowHeapOffset));
	cl->SetComputeRootConstantBufferView(RS::CB_PER_FRAME, m_pCbPerFrame->GetDefaultResource()->GetGPUVirtualAdress());
	cl->SetComputeRootConstantBufferView(RS::CB_PER_SCENE, m_pCbPerScene->GetDefaultResource()->GetGPUVirtualAdress());
	cl->SetComputeRootConstantBufferView(RS::CBV0, m_pCbCamera->GetDefaultResource()->GetGPUVirtualAdress());
	cl->SetComputeRootShaderResourceView(RS::SRV0, m_LightRawBuffers[LIGHT_TYPE::POINT_LIGHT]->shaderResource->GetUploadResource()->GetGPUVirtualAdress());

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


	// The hit groups section start after the miss shaders.
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


	// Set temporal buffers written to UAV
	for (unsigned int i = 0; i < m_Lights[LIGHT_TYPE::POINT_LIGHT].size(); i++)
	{
		cl->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
			m_LightTemporalResources[i][currentLightTemporalBuffer]->GetID3D12Resource1(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	}

	//DX12TEST(cl->DispatchRays(&desc), 0);
	cl->DispatchRays(&desc);

	

	, 0);
#pragma endregion RayTrace

	// Execute BlurTasks, output to m_LightTemporalResources
	GaussianSpatialAccumulation(cl, currentLightTemporalBuffer);

	// Execute ShadowBufferTask, output to m_ShadowBuffer
	temporalAccumulation(cl);

	FinalBlur(cl);

	// Calculate Light and output to m_Output
	lightningMergeTask(cl);

	// TAA
	TAATask(cl);
	m_DXTimer.Stop(cl, 1); m_DXTimer.ResolveQueryToCPU(cl, 1);




	// Copy outputResource to swapchain
	transition = CD3DX12_RESOURCE_BARRIER::Transition(
		m_pOutputResource->GetID3D12Resource1(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, // StateBefore
		D3D12_RESOURCE_STATE_COPY_SOURCE);	   // StateAfter
	cl->ResourceBarrier(1, &transition);


	transition = CD3DX12_RESOURCE_BARRIER::Transition(
		swapChainRenderTarget->GetResource()->GetID3D12Resource1(),
		D3D12_RESOURCE_STATE_PRESENT,	 // StateBefore
		D3D12_RESOURCE_STATE_COPY_DEST); // StateAfter
	cl->ResourceBarrier(1, &transition);


	cl->CopyResource(
		swapChainRenderTarget->GetResource()->GetID3D12Resource1(),	// Dest
		m_pOutputResource->GetID3D12Resource1());					// Source


	transition = CD3DX12_RESOURCE_BARRIER::Transition(
		swapChainRenderTarget->GetResource()->GetID3D12Resource1(),
		D3D12_RESOURCE_STATE_COPY_DEST, // StateBefore
		D3D12_RESOURCE_STATE_PRESENT);	// StateAfter
	cl->ResourceBarrier(1, &transition);

	transition = CD3DX12_RESOURCE_BARRIER::Transition(
		m_pOutputResource->GetID3D12Resource1(),
		D3D12_RESOURCE_STATE_COPY_SOURCE,		// StateBefore
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS);	// StateAfter
	cl->ResourceBarrier(1, &transition);


	cl->Close();
	ID3D12CommandList* cLists[] = { cl };
	m_CommandQueues[COMMAND_INTERFACE_TYPE::DIRECT_TYPE]->ExecuteCommandLists(1, cLists);
	/*------------------- Post draw stuff -------------------*/
	waitForGPU();

	OutputTestResults(dt);

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
			
			rc->tc->CreateResourceForWorldMatrix(m_pDevice5, m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]);
			
			// One resource for each mesh
			for (unsigned int i = 0; i < mc->GetNrOfMeshes(); i++)
			{
				rc->CB_PER_OBJECT_UPLOAD_RESOURCES.push_back(new Resource(
					m_pDevice5,
					sizeof(CB_PER_OBJECT_STRUCT),
					RESOURCE_TYPE::UPLOAD,
					L"CB_PER_OBJECT_UPLOAD_RESOURCE_" + std::to_wstring(i) + L"_" + mc->GetModelPath()));

				rc->CB_PER_OBJECT_DEFAULT_RESOURCES.push_back(new Resource(
					m_pDevice5,
					sizeof(CB_PER_OBJECT_STRUCT),
					RESOURCE_TYPE::DEFAULT,
					L"CB_PER_OBJECT_DEFAULT_RESOURCE_" + std::to_wstring(i) + L"_" + mc->GetModelPath()));
			}
			// Finally store the object in the in renderer so it can be drawn
			m_RenderComponents.push_back(rc);
		}
	}
}

void Renderer::InitDirectionalLightComponent(component::DirectionalLightComponent* component)
{
	// Save in m_pRenderer
	m_Lights[LIGHT_TYPE::DIRECTIONAL_LIGHT].push_back(component);
	
	// We also need to update the indexBuffer with lights if a light is added.
	// The buffer with indices is inside cbPerSceneData, which is updated in the following function:
	submitUploadPerSceneData();
}

void Renderer::InitPointLightComponent(component::PointLightComponent* component)
{
	// Assign CBV from the lightPool
	std::wstring resourceName = L"PointLight";

	// Bad solution!
	// Remove from copyPerFrameTask, and add it again at the end of the function, but with the updated data.
	CopyPerFrameTask* cpft = static_cast<CopyPerFrameTask*>(m_CopyTasks[COPY_TASK_TYPE::COPY_PER_FRAME]);
	cpft->ClearSpecific(m_LightRawBuffers[LIGHT_TYPE::POINT_LIGHT]->shaderResource->GetUploadResource());

	plRawBufferData.header.numLights++;

	PointLight pl = *(static_cast<PointLight*>(component->GetLightData()));
	plRawBufferData.pls.push_back(pl);

	m_LightRawBuffers[LIGHT_TYPE::POINT_LIGHT]->dataSizeVec =
	{
		{&plRawBufferData.header, sizeof(LightHeader)},
		{plRawBufferData.pls.data(), MAX_POINT_LIGHTS * sizeof(PointLight)}
	};

	submitToCpftAppend(m_LightRawBuffers[LIGHT_TYPE::POINT_LIGHT]);

	// Save in m_pRenderer
	m_Lights[LIGHT_TYPE::POINT_LIGHT].push_back(component);
}

void Renderer::InitSpotLightComponent(component::SpotLightComponent* component)
{
	// Assign CBV from the lightPool
	std::wstring resourceName = L"SpotLight";

	// Save in m_pRenderer
	m_Lights[LIGHT_TYPE::SPOT_LIGHT].push_back(component);
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
				delete m_RenderComponents[i]->CB_PER_OBJECT_DEFAULT_RESOURCES.at(j);
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
	for (Light* light: m_Lights[type])
	{
		component::DirectionalLightComponent* dlc = static_cast<component::DirectionalLightComponent*>(light);

		// Remove light if it matches the entity
		if (component == dlc)
		{
			// Finally remove from m_pRenderer
			m_Lights[type].erase(m_Lights[type].begin() + count);
			break;
		}
		count++;
	}
}

void Renderer::UnInitPointLightComponent(component::PointLightComponent* component)
{
	LIGHT_TYPE type = LIGHT_TYPE::POINT_LIGHT;
	unsigned int count = 0;
	for (Light* light : m_Lights[type])
	{
		component::PointLightComponent* plc = static_cast<component::PointLightComponent*>(light);

		// Remove light if it matches the entity
		if (component == plc)
		{
			// Finally remove from m_pRenderer
			m_Lights[type].erase(m_Lights[type].begin() + count);
			break;
		}
		count++;
	}
}

void Renderer::UnInitSpotLightComponent(component::SpotLightComponent* component)
{
	LIGHT_TYPE type = LIGHT_TYPE::SPOT_LIGHT;
	unsigned int count = 0;
	for (Light* light : m_Lights[type])
	{
		component::SpotLightComponent* slc = static_cast<component::SpotLightComponent*>(light);

		// Remove light if it matches the entity
		if (component == slc)
		{
			m_Lights[type].erase(m_Lights[type].begin() + count);
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

	BLModel* bLmodel = new BLModel;

	for (unsigned int i = 0; i < model->GetSize(); i++)
	{
		Mesh* mesh = model->GetMeshAt(i);

		// DXR
		bLmodel->vertexBuffers.push_back(std::make_pair(mesh->GetDefaultResourceVertices()->GetID3D12Resource1(), mesh->GetNumVertices()));
		bLmodel->indexBuffers.push_back(std::make_pair(mesh->GetDefaultResourceIndices()->GetID3D12Resource1(), mesh->GetNumIndices()));

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

	// Submit slotInfo (DXR)
	CopyOnDemandTask* codt = static_cast<CopyOnDemandTask*>(m_CopyTasks[COPY_TASK_TYPE::COPY_ON_DEMAND]);
	const void* data = static_cast<const void*>(model->m_SlotInfos.data());
	codt->Submit(&std::make_tuple(
		model->m_SlotInfoByteAdressBuffer->GetUploadResource(), 
		model->m_SlotInfoByteAdressBuffer->GetDefaultResource(),
		data));

	// DXR
	m_BottomLevelModels.push_back(bLmodel);
	CreateBottomLevelAS(&bLmodel);
	model->SetBottomLevelResult(bLmodel->ASbuffer->result->GetID3D12Resource1());

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

void Renderer::submitToCpftAppend(ShaderResource_ContinousMemory* shaderResource_contMem)
{
	CopyPerFrameTask* cpft = static_cast<CopyPerFrameTask*>(m_CopyTasks[COPY_TASK_TYPE::COPY_PER_FRAME]);
	cpft->SubmitContinousMemory(shaderResource_contMem);
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

void Renderer::CreateBottomLevelAS(BLModel** blModel)
{
	// reset vertexbuffers
	m_BottomLevelASGenerator = {};

	// Adding all vertex buffers and not transforming their position.
	for (unsigned int i = 0; i < (*blModel)->vertexBuffers.size(); i++)
	{
		std::pair<ID3D12Resource1*, uint32_t> vBuffer = (*blModel)->vertexBuffers.at(i);
		std::pair<ID3D12Resource1*, uint32_t> iBuffer = (*blModel)->indexBuffers.at(i);

		m_BottomLevelASGenerator.AddVertexBuffer(
			vBuffer.first, 0, vBuffer.second, sizeof(Vertex),
			iBuffer.first, 0, iBuffer.second, nullptr, 0, true);
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
	(*blModel)->ASbuffer = new AccelerationStructureBuffers();
	(*blModel)->ASbuffer->scratch = new Resource(
	m_pDevice5, scratchSizeInBytes,
	RESOURCE_TYPE::DEFAULT, L"scratchBottomLevel",
	D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	// Result
	(*blModel)->ASbuffer->result = new Resource(
		m_pDevice5, resultSizeInBytes,
		RESOURCE_TYPE::DEFAULT, L"resultBottomLevel",
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);
	
	// Build the acceleration structure. Note that this call integrates a barrier
	// on the generated AS, so that it can be used to compute a top-level AS right
	// after this method.
	m_BottomLevelASGenerator.Generate(m_pTempCommandInterface->GetCommandList(0), (*blModel)->ASbuffer->scratch->GetID3D12Resource1(),
		(*blModel)->ASbuffer->result->GetID3D12Resource1(), false, nullptr);
}

void Renderer::CreateTopLevelAS(std::vector<std::pair<ID3D12Resource1*, DirectX::XMMATRIX>>& instances)
{
	for (size_t i = 0; i < instances.size(); i++)
	{
		m_TopLevelAsGenerator.AddInstance(
			instances[i].first,
			instances[i].second, 
			static_cast<unsigned int>(i),
			static_cast<unsigned int>(0));	// One hitgroup for each instance
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
	// refitted in place.m_pTempCommandInterface
	m_TopLevelAsGenerator.Generate(m_pTempCommandInterface->GetCommandList(0),
		m_TopLevelASBuffers.scratch->GetID3D12Resource1(),
		m_TopLevelASBuffers.result->GetID3D12Resource1(),
		m_TopLevelASBuffers.instanceDesc->GetID3D12Resource1());
}

void Renderer::CreateAccelerationStructures()
{
	for (RenderComponent* rc : m_RenderComponents)
	{
		std::pair<ID3D12Resource1*, DirectX::XMMATRIX> pair = std::make_pair(rc->mc->GetModel()->GetBottomLevelResultP(), *rc->tc->GetTransform()->GetWorldMatrix());
		m_instances.push_back(pair);
	}

	CreateTopLevelAS(m_instances);

	// Flush the command list and wait for it to finish
	m_pTempCommandInterface->GetCommandList(0)->Close();
	ID3D12CommandList* ppCommandLists[] = { m_pTempCommandInterface->GetCommandList(0) };
	m_CommandQueues[COMMAND_INTERFACE_TYPE::DIRECT_TYPE]->ExecuteCommandLists(1, ppCommandLists);

	// Wait if the CPU is to far ahead of the gpu
	waitForGPU();
	//m_CommandQueues[COMMAND_INTERFACE_TYPE::DIRECT_TYPE]->Signal(m_pFenceFrame, m_FenceFrameValue);
	//waitForFrame(0);
	//m_FenceFrameValue++;
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

ID3D12RootSignature* Renderer::CreateShadowSignature()
{
	nv_helpers_dx12::RootSignatureGenerator rsc;
	return rsc.Generate(m_pDevice5, true);
}

ID3D12RootSignature* Renderer::CreateHitSignature()
{
	//-----------------------------------------------------------------------------
	nv_helpers_dx12::RootSignatureGenerator rsc;

	// ConstantBuffer with worldMatrix. Unique per Instance (b7, space3)
	rsc.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_CBV, 7, 3, 1);

	// ByteBuffer with SlotInfos. Unique per Model (t0, space5)
	rsc.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_SRV, 0, 5, 1);

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
	m_pRayGenShader = new Shader(L"../BeLuEngine/src/Renderer/DXR_Helpers/shaders/RayGen.hlsl",		SHADER_TYPE::DXR);
	//m_pHitShader	= new Shader(L"../BeLuEngine/src/Renderer/DXR_Helpers/shaders/Hit.hlsl",		SHADER_TYPE::DXR);
	//m_pMissShader	= new Shader(L"../BeLuEngine/src/Renderer/DXR_Helpers/shaders/Miss.hlsl",		SHADER_TYPE::DXR);
	m_pShadowShader = new Shader(L"../BeLuEngine/src/Renderer/DXR_Helpers/shaders/ShadowRay.hlsl",  SHADER_TYPE::DXR);

	  // In a way similar to DLLs, each library is associated with a number of
	  // exported symbols. This
	  // has to be done explicitly in the lines below. Note that a single library
	  // can contain an arbitrary number of symbols, whose semantic is given in HLSL
	  // using the [shader("xxx")] syntax
	pipeline.AddLibrary(m_pRayGenShader->GetBlob(), { L"RayGen" });
	//pipeline.AddLibrary(m_pHitShader->GetBlob(),	{ L"ClosestHit" });
	//pipeline.AddLibrary(m_pMissShader->GetBlob(),	{ L"Miss" });
	pipeline.AddLibrary(m_pShadowShader->GetBlob(), {  L"ShadowMiss" });

	// To be used, each DX12 shader needs a root signature defining which
	// parameters and buffers will be accessed.
	m_pRayGenSignature = CreateRayGenSignature();
	//m_pHitSignature = CreateHitSignature();
	//m_pMissSignature = CreateMissSignature();
	m_pShadowSignature = CreateMissSignature();

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
	//pipeline.AddHitGroup(L"HitGroup", L"ClosestHit");
	//pipeline.AddHitGroup(L"ShadowHitGroup", L"ShadowClosestHit");

	// The following section associates the root signature to each shader.Note
	// that we can explicitly show that some shaders share the same root signature
	// (eg. Miss and ShadowMiss). Note that the hit shaders are now only referred
	// to as hit groups, meaning that the underlying intersection, any-hit and
	// closest-hit shaders share the same root signature.
	pipeline.AddRootSignatureAssociation(m_pRayGenSignature, { L"RayGen" });
	//pipeline.AddRootSignatureAssociation(m_pHitSignature, { L"HitGroup" });
	//pipeline.AddRootSignatureAssociation(m_pHitSignature, { L"ShadowHitGroup" });
	pipeline.AddRootSignatureAssociation(m_pMissSignature, { L"ShadowMiss" });
	// The payload size defines the maximum size of the data carried by the rays,
	// ie. the the data
	// exchanged between shaders, such as the HitInfo structure in the HLSL code.
	// It is important to keep this value as low as possible as a too high value
	// would result in unnecessary memory consumption and cache trashing.
	pipeline.SetMaxPayloadSize(1 * sizeof(float)); // bool ShadowIsHit

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

void Renderer::temporalAccumulation(ID3D12GraphicsCommandList5* cl)
{
	m_ShadowBufferRenderTask->SetBackBufferIndex(0);
	m_ShadowBufferRenderTask->SetCommandInterfaceIndex(0);
	m_ShadowBufferRenderTask->SetHeapOffsets(m_DhIndexSoftShadowsUAV, m_DhIndexSoftShadowsBuffer);
	m_ShadowBufferRenderTask->SetPingPongLightResources(m_Lights[LIGHT_TYPE::POINT_LIGHT].size(), m_pShadowBufferPingPong);
	m_ShadowBufferRenderTask->Execute();
}

void Renderer::spatialAccumulation(ID3D12GraphicsCommandList5* cl)
{
	// Blur all light output
	m_GaussianBlurAllShadowsTask->SetPingPongResorcesToBlur(m_Lights[LIGHT_TYPE::POINT_LIGHT].size(), m_pShadowBufferPingPong);
	m_GaussianBlurAllShadowsTask->SetBackBufferIndex(0);
	m_GaussianBlurAllShadowsTask->SetCommandInterfaceIndex(0);
	m_GaussianBlurAllShadowsTask->Execute();
}

void Renderer::GaussianSpatialAccumulation(ID3D12GraphicsCommandList5* cl, unsigned int currentTemporalIndex)
{
	// Blur all light output
	std::vector<PingPongResource*> pingPongsTest;

	for (unsigned int i = 0; i < m_Lights[LIGHT_TYPE::POINT_LIGHT].size(); i++)
	{
		pingPongsTest.push_back(m_LightTemporalPingPong[i][currentTemporalIndex]);
	}

	m_GaussianBlurAllShadowsTask->SetPingPongResorcesToBlur(m_Lights[LIGHT_TYPE::POINT_LIGHT].size(), pingPongsTest.data());
	m_GaussianBlurAllShadowsTask->SetBackBufferIndex(0);
	m_GaussianBlurAllShadowsTask->SetCommandInterfaceIndex(0);
	m_GaussianBlurAllShadowsTask->Execute();

	// Set temporal buffers written to back
	for (unsigned int i = 0; i < m_Lights[LIGHT_TYPE::POINT_LIGHT].size(); i++)
	{
		cl->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
			m_LightTemporalResources[i][currentTemporalIndex]->GetID3D12Resource1(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
	}
}

void Renderer::FinalBlur(ID3D12GraphicsCommandList5* cl)
{
	// Blur all light output
	std::vector<PingPongResource*> pingPongsTest;

	for (unsigned int i = 0; i < m_Lights[LIGHT_TYPE::POINT_LIGHT].size(); i++)
	{
		pingPongsTest.push_back(m_pShadowBufferPingPong[i]);
	}

	m_FinalBlurAllShadowsTask->SetPingPongResorcesToBlur(m_Lights[LIGHT_TYPE::POINT_LIGHT].size(), pingPongsTest.data());
	m_FinalBlurAllShadowsTask->SetBackBufferIndex(0);
	m_FinalBlurAllShadowsTask->SetCommandInterfaceIndex(0);
	m_FinalBlurAllShadowsTask->Execute();

	// Wait for UAVS to get written 
	D3D12_RESOURCE_BARRIER rBarr = {};
	rBarr.Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_UAV;

	for (unsigned int i = 0; i < m_Lights[LIGHT_TYPE::POINT_LIGHT].size(); i++)
	{
		rBarr.UAV.pResource = m_pShadowBufferResource[i]->GetID3D12Resource1();
		cl->ResourceBarrier(1, &rBarr);
	}
}

void Renderer::lightningMergeTask(ID3D12GraphicsCommandList5* cl)
{
	// Reads from m_ShadowBuffers (which are spatiotemporal accumulated)
	// Then calculates lightning and shading.
	// Writes to m_pOutputResource, which is later copied to swapchain

	m_MergeLightningRenderTask->SetBackBufferIndex(0);
	m_MergeLightningRenderTask->SetCommandInterfaceIndex(0);

	unsigned int softShadowBufferOffset = m_DhIndexSoftShadowsBuffer;
	m_MergeLightningRenderTask->SetHeapOffsets(softShadowBufferOffset);

	cl->SetGraphicsRootDescriptorTable(RS::dtRaytracing, m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]->GetGPUHeapAt(m_DhIndexASOB));
	cl->SetGraphicsRootDescriptorTable(RS::dtSRV, m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]->GetGPUHeapAt(0)); // Read mesh
	cl->SetGraphicsRootDescriptorTable(RS::dtSRV2, m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]->GetGPUHeapAt(softShadowBufferOffset));
	cl->SetGraphicsRootShaderResourceView(RS::SRV0, m_LightRawBuffers[LIGHT_TYPE::POINT_LIGHT]->shaderResource->GetUploadResource()->GetGPUVirtualAdress());
	cl->SetGraphicsRootConstantBufferView(RS::CBV0, m_pCbCamera->GetDefaultResource()->GetGPUVirtualAdress());
	//unsigned int data = m_pMainDepthStencil->GetDSV()->GetDescriptorHeapIndex();
	unsigned int data[4];
	data[0] = m_pMainDepthStencil->GetSRV()->GetDescriptorHeapIndex();
	data[1] = m_GBufferNormal.srv->GetDescriptorHeapIndex();

	cl->SetGraphicsRoot32BitConstants(RS::RC_4, 2, data, 0);
	
	

	for (unsigned int i = 0; i < m_Lights[LIGHT_TYPE::POINT_LIGHT].size(); i++)
	{
		auto transition = CD3DX12_RESOURCE_BARRIER::Transition(
			m_pShadowBufferResource[i]->GetID3D12Resource1(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,		// StateBefore
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);	// StateAfter
		cl->ResourceBarrier(1, &transition);
	}

	m_MergeLightningRenderTask->Execute();

	for (unsigned int i = 0; i < m_Lights[LIGHT_TYPE::POINT_LIGHT].size(); i++)
	{
		auto transition = CD3DX12_RESOURCE_BARRIER::Transition(
			m_pShadowBufferResource[i]->GetID3D12Resource1(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,		// StateBefore
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS);	// StateAfter
		cl->ResourceBarrier(1, &transition);
	}
}

void Renderer::TAATask(ID3D12GraphicsCommandList5* cl)
{
	m_TAARenderTask->SetBackBufferIndex(0);
	m_TAARenderTask->SetCommandInterfaceIndex(0);
	m_TAARenderTask->Execute();
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
	resDesc.Width = resolutionWidth; //m_pWindow->GetScreenWidth();
	resDesc.Height = resolutionHeight;// m_pWindow->GetScreenHeight();
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.MipLevels = 1;
	resDesc.SampleDesc.Count = 1;

	m_pOutputResource = new Resource(m_pDevice5, &resDesc, nullptr,
		L"OutputResource", D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
}

void Renderer::CreateShaderResourceHeap()
{
	// Create a SRV/UAV/CBV descriptor heap. We need 2 entries - 1 UAV for the
	// raytracing output and 1 SRV for the TLAS

	// Get a handle to the heap memory on the CPU side, to be able to write the
	// descriptors directly
	m_DhIndexASOB = m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]->GetNextDescriptorHeapIndex(1);
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]->GetCPUHeapAt(m_DhIndexASOB);

	// Create the UAV. Based on the root signature we created it is the first
	// entry. The Create*View methods write the view information directly into
	// srvHandle
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	m_pDevice5->CreateUnorderedAccessView(m_pOutputResource->GetID3D12Resource1(), nullptr, &uavDesc, srvHandle);

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
	//m_SbtHelper.AddMissProgram(L"Miss", {});
	m_SbtHelper.AddMissProgram(L"ShadowMiss", {});

	// Adding the triangle hit shader
	//for (RenderComponent* rc : m_RenderComponents)
	//{
		//m_SbtHelper.AddHitGroup(L"HitGroup", 
		//	{
		//		(void*)rc->tc->GetMatrixUploadResource()->GetGPUVirtualAdress(), // Unique per instance
		//		(void*)rc->mc->GetModel()->GetByteAdressInfoDXR()->GetDefaultResource()->GetGPUVirtualAdress()	// Unique per model
		//	});	

		//m_SbtHelper.AddHitGroup(L"ShadowHitGroup", {});
	//}


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

void Renderer::CreateSoftShadowLightResources()
{
	auto format = DXGI_FORMAT_R16G16B16A16_FLOAT;

	// Shadow Buffer
	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.DepthOrArraySize = 1;
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	// The backbuffer is actually DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, but sRGB
	// formats cannot be used with UAVs. For accuracy we should convert to sRGB
	// ourselves in the shader
	resDesc.Format = format;

	resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	resDesc.Width = resolutionWidth; //m_pWindow->GetScreenWidth();
	resDesc.Height = resolutionHeight;// m_pWindow->GetScreenHeight();
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.MipLevels = 1;
	resDesc.SampleDesc.Count = 1;

	

	// Create SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	// Create UAV
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = format;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;


	// save index in heap
	m_DhIndexSoftShadowsBuffer = m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]->GetNextDescriptorHeapIndex(0);

	for (unsigned int i = 0; i < MAX_POINT_LIGHTS; i++)
	{
		m_pShadowBufferResource[i] = new Resource(m_pDevice5, &resDesc, nullptr,
			L"m_pShadowBufferResource", D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		m_pShadowBufferPingPong[i] = new PingPongResource(m_pShadowBufferResource[i], m_pDevice5,
			m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV],
			&srvDesc, &uavDesc);
	}

	// save index in heap
	m_DhIndexSoftShadowsUAV = m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]->GetNextDescriptorHeapIndex(0);

	// Create UAV resources for each temporal buffer
	for (unsigned int z = 0; z < NUM_TEMPORAL_BUFFERS + 1; z++)
	{
		// Create UAV resources for each light
		for (unsigned int i = 0; i < MAX_POINT_LIGHTS; i++)
		{
			m_LightTemporalResources[i][z] = new Resource(m_pDevice5, &resDesc, nullptr,
				L"m_LightTemporalResources", D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			m_LightTemporalPingPong[i][z] = new PingPongResource(m_LightTemporalResources[i][z], m_pDevice5, m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV], &srvDesc, &uavDesc);
		}
	}
}

void Renderer::CreateTAAResource()
{
	auto format = DXGI_FORMAT_R16G16B16A16_FLOAT;

	// TAA Buffer
	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.DepthOrArraySize = 1;
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	// The backbuffer is actually DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, but sRGB
	// formats cannot be used with UAVs. For accuracy we should convert to sRGB
	// ourselves in the shader
	resDesc.Format = format;

	resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	resDesc.Width = resolutionWidth; //m_pWindow->GetScreenWidth();
	resDesc.Height = resolutionHeight;// m_pWindow->GetScreenHeight();
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.MipLevels = 1;
	resDesc.SampleDesc.Count = 1;

	// Create SRV desc
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	// Create desc
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = format;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;


	m_pTAAResource = new Resource(m_pDevice5, &resDesc, nullptr,
		L"m_pTAAResource", D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	m_pTAAPingPong = new PingPongResource(m_pTAAResource, m_pDevice5,
		m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV],
		&srvDesc, &uavDesc);
}

void Renderer::createRawBuffersForLights()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC rawBufferDesc = {};
	rawBufferDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	rawBufferDesc.Buffer.FirstElement = 0;
	rawBufferDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	rawBufferDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	rawBufferDesc.Buffer.NumElements = MAX_POINT_LIGHTS;
	rawBufferDesc.Buffer.StructureByteStride = 0;
	rawBufferDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;

	m_LightRawBuffers[LIGHT_TYPE::POINT_LIGHT] = new ShaderResource_ContinousMemory();

	m_LightRawBuffers[LIGHT_TYPE::POINT_LIGHT]->shaderResource = new ShaderResource(
		m_pDevice5,
		sizeof(LightHeader) + sizeof(PointLight) * MAX_POINT_LIGHTS,
		L"_RAWBUFFER_POINTLIGHTS",
		&rawBufferDesc,
		m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]);

	//int a = m_LightRawBuffers[LIGHT_TYPE::POINT_LIGHT]->shaderResource->GetSRV()->GetDescriptorHeapIndex();
	plRawBufferData.header = {};
}

void Renderer::setRenderTasksPrimaryCamera()
{
	m_RenderTasks[RENDER_TASK_TYPE::DEPTH_PRE_PASS]->SetCamera(m_pScenePrimaryCamera);
	m_RenderTasks[RENDER_TASK_TYPE::GBUFFER_PASS  ]->SetCamera(m_pScenePrimaryCamera);
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

			system("nvidia-smi -q --xml-format -f nvidia-smi.txt");

			m_GPUName = to_string(adapterDesc.Description);
			m_DriverVersion = getDriverVersion();
			SetResultsFileName();
			
			BL_LOG("Adapter: %s\n", m_GPUName.c_str());
			BL_LOG("Driver Version: %s\n", m_DriverVersion.c_str());

			D3D12_FEATURE_DATA_D3D12_OPTIONS5 features5 = {};
			HRESULT hr = pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &features5, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS5));
			SAFE_RELEASE(&pDevice);
			
			if (SUCCEEDED(hr))
			{
				if (features5.RaytracingTier == D3D12_RAYTRACING_TIER_1_1)
				{
					BL_LOG("Raytracing tier 1.1 supported!\n");
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
	//unsigned int resolutionWidth = m_pWindow->GetScreenWidth();
	//unsigned int resolutionHeight = m_pWindow->GetScreenHeight();

	m_pSwapChain = new SwapChain(
		m_pDevice5,
		m_pWindow->GetHwnd(),
		resolutionWidth, resolutionHeight,
		m_CommandQueues[COMMAND_INTERFACE_TYPE::DIRECT_TYPE],
		m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::RTV],
		m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]);
}

std::string Renderer::getDriverVersion()
{
	// Open nvidia-smi.txt
	std::ifstream file;
	file.open("nvidia-smi.txt", std::ios_base::in);

	const int buf_size = 5000;
	char buffer[buf_size];
	file.read(buffer, buf_size);

	// find driverversion
	std::string bufferString = std::string(buffer);
	const char findStr[] = "<driver_version>";
	const char findStrEnd[] = "</driver_version>";
	unsigned int findLength = std::strlen(findStr);

	unsigned int foundIndexStart, foundIndexEnd;
	foundIndexStart = bufferString.find(findStr, 0);

	foundIndexEnd = bufferString.find(findStrEnd, foundIndexStart + findLength);

	std::string driverVersion = std::string(buffer + foundIndexStart + findLength, buffer + foundIndexEnd);
	
	return driverVersion;
}

void Renderer::createGBufferRenderTargets()
{
	//unsigned int resolutionWidth = m_pWindow->GetScreenWidth();
	//unsigned int resolutionHeight = m_pWindow->GetScreenHeight();

	DXGI_FORMAT textureFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;// TODO: change to more proper format

	D3D12_RESOURCE_DESC rDesc = {};
	rDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	rDesc.Alignment = 0;
	rDesc.Width = resolutionWidth;
	rDesc.Height = resolutionHeight;
	rDesc.DepthOrArraySize = 1;
	rDesc.MipLevels = 1;
	rDesc.Format = textureFormat;	
	rDesc.SampleDesc.Count = 1;
	rDesc.SampleDesc.Quality = 0;
	rDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	rDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = textureFormat;
	clearValue.Color[0] = 0.0f;
	clearValue.Color[1] = 0.0f;
	clearValue.Color[2] = 0.0f;
	clearValue.Color[3] = 1.0f;

	m_GBufferNormal.resource = new Resource(m_pDevice5, &rDesc, &clearValue, L"GBufferNormalResource", D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	
	// Create SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = textureFormat;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	m_GBufferNormal.srv = new ShaderResourceView(m_pDevice5, m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV], &srvDesc, m_GBufferNormal.resource);

	// Create RTV
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	m_GBufferNormal.rtv = new RenderTargetView(
		m_pDevice5,
		resolutionWidth, resolutionHeight,
		m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::RTV],
		&rtvDesc,
		m_GBufferNormal.resource,
		true);
}

void Renderer::createMainDSV()
{
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	m_pMainDepthStencil = new DepthStencil(
		m_pDevice5,
		resolutionWidth, resolutionHeight,
		L"MainDSV",
		&dsvDesc,
		m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::DSV],
		m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]);

	// SRV, to read from depth. (Used mainly to start the rays from the worldposition instead of the camera)
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	srvDesc.Texture2D.PlaneSlice = 0;
	
	m_pDepthBufferSRV = new ShaderResourceView(m_pDevice5, m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV], &srvDesc, m_pMainDepthStencil->GetDefaultResource());
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
	static_cast<DepthRenderTask*>(DepthPrePassRenderTask)->SetCommandInterface(m_pTempCommandInterface);

#pragma endregion DepthPrePass

#pragma region GBufferPass
	/* Forward rendering without stencil testing */
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsdGBufferRender = {};
	gpsdGBufferRender.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	// RenderTarget (Currently only normals)
	gpsdGBufferRender.NumRenderTargets = 1;
	gpsdGBufferRender.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	// Depthstencil usage
	gpsdGBufferRender.SampleDesc.Count = 1;
	gpsdGBufferRender.SampleMask = UINT_MAX;
	// Rasterizer behaviour
	gpsdGBufferRender.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	gpsdGBufferRender.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	gpsdGBufferRender.RasterizerState.FrontCounterClockwise = false;

	// Specify Blend descriptions
	D3D12_RENDER_TARGET_BLEND_DESC defaultRTdesc = {
		false, false,
		D3D12_BLEND_ZERO, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP, D3D12_COLOR_WRITE_ENABLE_ALL };
	for (unsigned int i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
		gpsdGBufferRender.BlendState.RenderTarget[i] = defaultRTdesc;

	// Depth descriptor
	D3D12_DEPTH_STENCIL_DESC dsd = {};
	dsd.DepthEnable = true;
	dsd.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	dsd.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	// DepthStencil
	dsd.StencilEnable = false;
	gpsdGBufferRender.DepthStencilState = dsd;
	gpsdGBufferRender.DSVFormat = m_pMainDepthStencil->GetDSV()->GetDXGIFormat();

	// All settings are done, now give it to the renderTask
	std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*> gpsdGBufferVector;
	gpsdGBufferVector.push_back(&gpsdGBufferRender);

	RenderTask* gBufferRenderTask = new GBufferRenderTask(
		m_pDevice5,
		m_pRootSignature,
		L"GBufferVertex.hlsl", L"GBufferPixel.hlsl",
		&gpsdGBufferVector,
		L"GBufferPSO");

	//gBufferRenderTask->AddResource("cbPerFrame", m_pCbPerFrame->GetDefaultResource());
	//gBufferRenderTask->AddResource("cbPerScene", m_pCbPerScene->GetDefaultResource());
	gBufferRenderTask->AddRenderTargetView("NormalRTV", m_GBufferNormal.rtv);
	gBufferRenderTask->SetMainDepthStencil(m_pMainDepthStencil);
	gBufferRenderTask->SetDescriptorHeaps(m_DescriptorHeaps);
	static_cast<GBufferRenderTask*>(gBufferRenderTask)->SetCommandInterface(m_pTempCommandInterface);
#pragma endregion GBufferPass

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
	defaultRTdesc = {
		false, false,
		D3D12_BLEND_ZERO, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP, D3D12_COLOR_WRITE_ENABLE_ALL };
	for (unsigned int i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
		gpsdForwardRender.BlendState.RenderTarget[i] = defaultRTdesc;

	// Depth descriptor
	dsd = {};
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

	m_pTempInlinePixelUploadResource = new Resource(
		m_pDevice5,
		sizeof(CB_PER_OBJECT_STRUCT),
		RESOURCE_TYPE::UPLOAD,
		L"CB_PER_OBJECT_UPLOAD_RESOURCE_tempPixelInline");

	forwardRenderTask->AddResource("cbPerFrame", m_pCbPerFrame->GetDefaultResource());
	forwardRenderTask->AddResource("cbPerScene", m_pCbPerScene->GetDefaultResource());
	forwardRenderTask->SetMainDepthStencil(m_pMainDepthStencil);
	forwardRenderTask->SetSwapChain(m_pSwapChain);
	forwardRenderTask->SetDescriptorHeaps(m_DescriptorHeaps);

	forwardRenderTask->AddShaderResourceView("GBufferNormal", m_GBufferNormal.srv);

#pragma endregion ForwardRendering

#pragma region ComputeAndCopyTasks

	// Compute
	// ComputeTasks
	std::vector<std::pair<std::wstring, std::wstring>> csNamePSOName;
	csNamePSOName.push_back(std::make_pair(L"InlineComputeRT.hlsl", L"InlineComputeRTPSO"));

	ComputeTask* inlineRTComputeTask = new InlineRTComputeTask(
		m_pDevice5, m_pRootSignature,
		csNamePSOName,
		COMMAND_INTERFACE_TYPE::DIRECT_TYPE);

	m_IRTNumThreadGroupsX = static_cast<unsigned int>(ceilf(static_cast<float>(resolutionWidth) / m_ThreadsPerGroup));;
	m_IRTNumThreadGroupsY = resolutionHeight;// m_pWindow->GetScreenHeight();
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
	
	m_ComputeTasks[COMPUTE_TASK_TYPE::INLINE_RT] = inlineRTComputeTask;

	/* ------------------------- DirectQueue Tasks ---------------------- */
	m_RenderTasks[RENDER_TASK_TYPE::DEPTH_PRE_PASS] = DepthPrePassRenderTask;
	m_RenderTasks[RENDER_TASK_TYPE::GBUFFER_PASS] = gBufferRenderTask;
	m_RenderTasks[RENDER_TASK_TYPE::FORWARD_RENDER] = forwardRenderTask;

	// Pushback in the order of execution
	// Copy on demand (textures, meshes etc..)
	for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		m_DirectCommandLists[i].push_back(copyOnDemandTask->GetCommandInterface()->GetCommandList(i));
		m_DXRCodtCommandLists[i] = copyOnDemandTask->GetCommandInterface()->GetCommandList(i);
	}

	// Copy per frame
	for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		m_DirectCommandLists[i].push_back(copyPerFrameTask->GetCommandInterface()->GetCommandList(i));
		m_DXRCpftCommandLists[i] = copyPerFrameTask->GetCommandInterface()->GetCommandList(i);
	}

	// Depth prepass
	for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		m_DirectCommandLists[i].push_back(DepthPrePassRenderTask->GetCommandInterface()->GetCommandList(i));
		m_DepthPrePassCommandLists[i] = DepthPrePassRenderTask->GetCommandInterface()->GetCommandList(i);
	}

	// GBuffer
	for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		//m_DirectCommandLists[i].push_back(gBufferRenderTask->GetCommandInterface()->GetCommandList(i));
		m_GBufferCommandLists[i] = gBufferRenderTask->GetCommandInterface()->GetCommandList(i);
	}

	for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		m_DirectCommandLists[i].push_back(forwardRenderTask->GetCommandInterface()->GetCommandList(i));
	}
}

void Renderer::setRenderTasksRenderComponents()
{
	m_RenderTasks[RENDER_TASK_TYPE::DEPTH_PRE_PASS]->SetRenderComponents(&m_RenderComponents);
	m_RenderTasks[RENDER_TASK_TYPE::GBUFFER_PASS  ]->SetRenderComponents(&m_RenderComponents);
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
	m_pCbPerSceneData->pointLightRawBufferIndex = m_LightRawBuffers[LIGHT_TYPE::POINT_LIGHT]->shaderResource->GetSRV()->GetDescriptorHeapIndex();
	m_pCbPerSceneData->depthBufferIndex = m_pDepthBufferSRV->GetDescriptorHeapIndex();
	m_pCbPerSceneData->gBufferNormalIndex = m_GBufferNormal.srv->GetDescriptorHeapIndex();
	m_pCbPerSceneData->spp = 1;


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

		const void* data2 = static_cast<void*>(m_pCbCameraData);
		cpft->Submit(&std::tuple(m_pCbCamera->GetUploadResource(), m_pCbCamera->GetDefaultResource(), data2));
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
		// Earlier it read from options. now just set
		width = 400;
		height = 300;
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
