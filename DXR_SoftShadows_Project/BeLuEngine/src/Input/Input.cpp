#include "Input.h"
#include "..\Events\EventBus.h"

Input& Input::GetInstance()
{
	static Input instance;

	return instance;
}

void Input::RegisterDevices(const HWND* hWnd)
{
	static RAWINPUTDEVICE m_Rid[2];

	// Register mouse
	m_Rid[0].usUsagePage = 0x01;
	m_Rid[0].usUsage = 0x02;
	m_Rid[0].dwFlags = 0;
	m_Rid[0].hwndTarget = *hWnd;

	// Register keyboard
	m_Rid[1].usUsagePage = 0x01;
	m_Rid[1].usUsage = 0x06;
	m_Rid[1].dwFlags = 0;
	m_Rid[1].hwndTarget = *hWnd;

	if (RegisterRawInputDevices(m_Rid, 2, sizeof(m_Rid[0])) == FALSE)
	{
		Log::PrintSeverity(Log::Severity::CRITICAL,"Device registration error: %f\n", GetLastError());
	}
}

void Input::SetKeyState(SCAN_CODES key, bool pressed)
{
	// If this is true, the key is not currently being pressed. Meaning that its the first time we start to move the camera
	// If this is false, the keyState is currently true. Which means that it is already being pressed, and an event should not be published.
	bool justPressed = !m_KeyState[key];

	m_KeyState[key] = pressed;
	if (key == SCAN_CODES::W ||
		key == SCAN_CODES::A ||
		key == SCAN_CODES::S ||
		key == SCAN_CODES::D ||
		key == SCAN_CODES::R ||
		key == SCAN_CODES::F)
	{
		// Move Camera (when pressing buttons)
		if (justPressed == true)
		{
			EventBus::GetInstance().Publish(&MovementInput(key, true));
		}
		// Stop moving (when releasing buttons)
		else if (pressed == false)
		{
			EventBus::GetInstance().Publish(&MovementInput(key, false));
		}
	}
}

void Input::SetMouseButtonState(MOUSE_BUTTON button, bool pressed)
{
	m_MouseButtonState[button] = pressed;
	switch (pressed) {
	case true:	// Pressed
		EventBus::GetInstance().Publish(&MouseClick(button, pressed));
		break;
	case false:	// Released
		EventBus::GetInstance().Publish(&MouseRelease(button, pressed));
		break;
	}
}

void Input::SetMouseScroll(short scrollAmount)
{
	int mouseScroll = static_cast<int>(scrollAmount > 0) * 2 - 1;
	EventBus::GetInstance().Publish(&MouseScroll(mouseScroll));
}

void Input::SetMouseMovement(int x, int y)
{
	EventBus::GetInstance().Publish(&MouseMovement(x, y));
}

bool Input::GetKeyState(SCAN_CODES key)
{
	return m_KeyState[key];
}

bool Input::GetMouseButtonState(MOUSE_BUTTON button)
{
	return m_MouseButtonState[button];
}

Input::Input()
{
}
