#include "BasicHeader.hpp"
#include "WinVKInput.hpp"

using namespace EngineCore;

namespace EngineCore
{
    auto GetPlatformMapping() -> const array<vkeyt, 256> &; // from WinVirtualKeysMapping.cpp
}

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
            controlsQueue.Enqueue(DeviceType::MouseKeyboard, ControlAction::MouseMove{deltaX, deltaY});

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

		vkeyt key = GetPlatformMapping()[wParam];

        controlsQueue.Enqueue(DeviceType::MouseKeyboard, ControlAction::Key{key, msg == WM_KEYDOWN ? ControlAction::Key::KeyState::Pressed : ControlAction::Key::KeyState::Released});
	}
	else
	{
		HARDBREAK;
	}
}
