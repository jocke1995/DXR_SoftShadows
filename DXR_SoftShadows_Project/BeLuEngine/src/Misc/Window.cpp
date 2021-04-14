#include "stdafx.h"
#include "Window.h"

#include "../Input/Input.h"

// callback function for windows messages
LRESULT CALLBACK WndProc(HWND hWnd, unsigned int msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_SYSKEYDOWN: // alt+enter
		if (wParam == VK_RETURN && (lParam & 0x60000000) == 0x20000000)
		{
			//EventBus::GetInstance().Publish(&WindowChange());
		}
		return 0;
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
		{
			//if (MessageBox(0, L"Are you sure you want to exit?", L"Exit", MB_YESNO | MB_ICONQUESTION) == IDYES)
			//{
			DestroyWindow(hWnd);
			//}
		}
		// Temp to create objects during runtime
		if (wParam == VK_SPACE)
		{
			spacePressed = true;
		}
		if (wParam == VK_TAB)
		{
			tabPressed = true;
		}
		
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	case WM_INPUT:
		unsigned int dwSize = 0;

		GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
		unsigned char* charArr = new unsigned char[dwSize];
		if (charArr == nullptr)
		{
			return 0;
		}

		if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, charArr, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize)
		{
			BL_LOG_CRITICAL("GetRawInputData does not return correct size !\n");
		}

#pragma region HandleInput
		// Check for input and set states in the Input class
		RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(charArr);
		if (raw->header.dwType == RIM_TYPEKEYBOARD)
		{
			RAWKEYBOARD inputData = raw->data.keyboard;

			int modifier = (inputData.Flags / 2 + 1) * 0x100;
			SCAN_CODES key = static_cast<SCAN_CODES>(inputData.MakeCode + modifier);

			Input::GetInstance().SetKeyState(key, !(inputData.Flags % 2));
		}
		else if (raw->header.dwType == RIM_TYPEMOUSE)
		{
			auto inputData = raw->data.mouse;
			MOUSE_BUTTON button = static_cast<MOUSE_BUTTON>(inputData.usButtonFlags);

			//switch (button)
			//{
			//case MOUSE_BUTTON::WHEEL:
			//	Input::GetInstance().SetMouseScroll(inputData.usButtonData);
			//	break;
			//case MOUSE_BUTTON::LEFT_DOWN:
			//	Input::GetInstance().SetMouseButtonState(button, true);
			//	break;
			//case MOUSE_BUTTON::MIDDLE_DOWN:
			//case MOUSE_BUTTON::RIGHT_DOWN:
			//	Input::GetInstance().SetMouseButtonState(button, true);
			//	break;
			//case MOUSE_BUTTON::LEFT_UP:
			//	button = static_cast<MOUSE_BUTTON>(static_cast<int>(button) / 2);
			//	Input::GetInstance().SetMouseButtonState(button, false);
			//	break;
			//case MOUSE_BUTTON::MIDDLE_UP:
			//case MOUSE_BUTTON::RIGHT_UP:
			//	button = static_cast<MOUSE_BUTTON>(static_cast<int>(button) / 2);
			//	Input::GetInstance().SetMouseButtonState(button, false);
			//	break;
			//default:
			//	break;
			//}

			Input::GetInstance().SetMouseMovement(inputData.lLastX, inputData.lLastY);

			// Enable this to lock mouse inside window
			//RECT win;
			//GetWindowRect(hWnd, &win);
			//ClipCursor(&win);
		}
#pragma endregion HandleInput
		delete[] charArr;

		return 0;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

Window::Window(
	HINSTANCE hInstance,
	int nCmdShow,
	bool windowedFullScreen,
	int screenWidth, int screenHeight,
	LPCTSTR windowName, LPCTSTR windowTitle)
{
	m_ScreenWidth = screenWidth;
	m_ScreenHeight = screenHeight;
	m_WindowedFullScreen = windowedFullScreen;
	m_WindowName = windowName;
	m_WindowTitle = windowTitle;

	initWindow(hInstance, nCmdShow);

	m_ShutDown = false;

	// Set this to true to Show cursor, dont forget to unlock cursor in ExitWindow()
	ShowCursor(false);
	SetCursorPos(m_ScreenWidth / 2, m_ScreenHeight / 2);
}


Window::~Window()
{

}

void Window::SetWindowTitle(std::wstring newTitle)
{
	SetWindowTextW(m_Hwnd, newTitle.c_str());
}

bool Window::IsFullScreen() const
{
	return m_WindowedFullScreen;
}

int Window::GetScreenWidth() const
{
	return m_ScreenWidth;
}

int Window::GetScreenHeight() const
{
	return m_ScreenHeight;
}

const HWND* Window::GetHwnd() const
{
	return &m_Hwnd;
}

void Window::SetScreenWidth(int width)
{
	m_ScreenWidth = width;
}

void Window::SetScreenHeight(int height)
{
	m_ScreenHeight = height;
}

bool Window::ExitWindow()
{
	//SetCursorPos(m_ScreenWidth / 2, m_ScreenHeight / 2);

	bool closeWindow = m_ShutDown;
	MSG msg = { 0 };

	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);

		if (msg.message == WM_QUIT)
		{
			m_ShutDown = true;
		}
	}
	return closeWindow;
}

bool Window::WasSpacePressed()
{
	if (spacePressed == true)
	{
		spacePressed = false;
		return true;
	}
	return false;
}

bool Window::WasTabPressed()
{
	if (tabPressed == true)
	{
		tabPressed = false;
		return true;
	}
	return false;
}

bool Window::initWindow(HINSTANCE hInstance, int nCmdShow)
{
	HMONITOR hmon = MonitorFromWindow(m_Hwnd, MONITOR_DEFAULTTONEAREST);
	MONITORINFO mi = { sizeof(mi) };
	GetMonitorInfo(hmon, &mi);

	int width = mi.rcMonitor.right - mi.rcMonitor.left;
	int height = mi.rcMonitor.bottom - mi.rcMonitor.top;

	if (m_WindowedFullScreen || (width < m_ScreenWidth || height < m_ScreenHeight))
	{
		m_ScreenWidth = width;
		m_ScreenHeight = height;
	}

	WNDCLASSEX wc;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;	// Device Context
	wc.lpfnWndProc = WndProc;	// Callback, Event catcher
	wc.cbClsExtra = NULL;
	wc.cbWndExtra = NULL;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 2);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = m_WindowName;
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	if (!RegisterClassEx(&wc))
	{
		MessageBox(NULL, L"Error registering class", L"Error", MB_OK | MB_ICONERROR);
		return false;
	}

	// This structure describes the window
	m_Hwnd = CreateWindowEx(NULL,
		m_WindowName,
		m_WindowTitle,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		m_ScreenWidth, m_ScreenHeight,
		NULL,
		NULL,
		hInstance,
		NULL);

	// If the windowhandle was unsuccesful
	if (!m_Hwnd)
	{
		MessageBox(NULL, L"Error creating window", L"Error", MB_OK | MB_ICONERROR);
		return false;
	}

	// Remove the topbar of the window if we are in fullscreen
	SetWindowLong(m_Hwnd, GWL_STYLE, 0);

	ShowWindow(m_Hwnd, nCmdShow);
	UpdateWindow(m_Hwnd);

	return true;
}
