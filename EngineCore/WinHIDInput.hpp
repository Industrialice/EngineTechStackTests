#pragma once

#include "KeyController.hpp"

namespace EngineCore
{
	class HIDInput
	{
		HWND _window = 0;

	public:
		~HIDInput();
		bool Register(HWND hwnd);
		void Unregister();
		void Dispatch(ControlsQueue &controlsQueue, HWND hwnd, WPARAM wParam, LPARAM lParam);
	};
}