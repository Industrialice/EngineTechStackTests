#include "BasicHeader.hpp"
#include "WinVKInput.hpp"

using namespace EngineCore;

void VKInput::Dispatch(ControlsQueue &controlsQueue, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_SETCURSOR || msg == WM_MOUSEMOVE)
	{
		POINTS mousePos = MAKEPOINTS(lParam);

		if (_mousePositionDefined == false)
		{
			_lastMouseX = mousePos.x;
			_lastMouseY = mousePos.y;
			_mousePositionDefined = true;
		}

		i32 deltaX = mousePos.x - _lastMouseX;
		i32 deltaY = mousePos.y - _lastMouseY;

		if (deltaX || deltaY)
		{
			controlsQueue.EnqueueMouseMove(DeviceType::MouseKeyboard0, deltaX, deltaY);

            _lastMouseX = mousePos.x;
            _lastMouseY = mousePos.y;
		}
	}
	else if (msg == WM_KEYDOWN || msg == WM_KEYUP)
	{
		if (wParam > 255)
		{
			SOFTBREAK;
			return;
		}

		vkey_t key = GetPlatformMapping()[wParam];

		controlsQueue.EnqueueKey(DeviceType::MouseKeyboard0, key, msg == WM_KEYDOWN ? ControlAction::Key::KeyStateType::Pressed : ControlAction::Key::KeyStateType::Released);
	}
	else
	{
		HARDBREAK;
	}
}
