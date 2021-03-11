#ifndef ENGINE_H
#define ENGINE_H

#include "GameIncludes.h"

class BeLuEngine
{
public:
	BeLuEngine();
	~BeLuEngine();

	void Init(HINSTANCE hInstance, int nCmdShow, ApplicationParameters* params = nullptr);

	const ApplicationParameters* GetApplicationParameters() const;

	Window* const GetWindow() const;
	Timer* const GetTimer() const;
	ThreadPool* const GetThreadPool() const;

	Renderer* const GetRenderer() const;
	SceneManager* const GetSceneHandler() const;

private:
	ApplicationParameters m_ApplicationParams;

	Window* m_pWindow = nullptr;
	Timer* m_pTimer = nullptr;
	ThreadPool* m_pThreadPool = nullptr;

	Renderer* m_pRenderer = nullptr;
	SceneManager* m_pSceneManager = nullptr;
};

#endif
