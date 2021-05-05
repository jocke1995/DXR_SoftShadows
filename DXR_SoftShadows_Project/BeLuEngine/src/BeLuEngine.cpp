#include "stdafx.h"
#include "BeLuEngine.h"

#define SCREENWIDTH 1280
#define SCREENHEIGHT 720


BeLuEngine::BeLuEngine()
{
	
}

BeLuEngine::~BeLuEngine()
{
	delete m_pWindow;
	delete m_pTimer;

	m_pSceneManager->deleteSceneManager();
	m_pRenderer->deleteRenderer();
}

void BeLuEngine::Init(HINSTANCE hInstance, int nCmdShow, ApplicationParameters* params)
{
	m_ApplicationParams = *params;

	// Window values
	bool windowedFullscreen = false;
	int windowWidth = SCREENWIDTH;
	int windowHeight = SCREENHEIGHT;

	// Misc
	m_pWindow = new Window(hInstance, nCmdShow, windowedFullscreen, windowWidth, windowHeight);
	m_pTimer = new Timer(m_pWindow);

	// Sub-engines
	m_pRenderer = &Renderer::GetInstance();

	m_pRenderer->SetQuitOnFinish(m_ApplicationParams.quitOnFinish); // Quits testing after num tests
	m_pRenderer->SetRTType(m_ApplicationParams.RayTracingType);
	m_pRenderer->SetNumLights(m_ApplicationParams.numLights);
	m_pRenderer->SetSceneName(m_ApplicationParams.scene);

	m_pRenderer->InitD3D12(m_pWindow, hInstance, m_pThreadPool);

	// ECS
	m_pSceneManager = &SceneManager::GetInstance();

	Input::GetInstance().RegisterDevices(m_pWindow->GetHwnd());
}

const ApplicationParameters* BeLuEngine::GetApplicationParameters() const
{
	return &m_ApplicationParams;
}

Window* const BeLuEngine::GetWindow() const
{
	return m_pWindow;
}

Timer* const BeLuEngine::GetTimer() const
{
	return m_pTimer;
}

ThreadPool* const BeLuEngine::GetThreadPool() const
{
	return m_pThreadPool;
}

SceneManager* const BeLuEngine::GetSceneHandler() const
{
	return m_pSceneManager;
}

Renderer* const BeLuEngine::GetRenderer() const
{
	return m_pRenderer;
}
