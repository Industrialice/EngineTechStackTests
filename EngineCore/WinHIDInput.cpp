#include "BasicHeader.hpp"
#include "WinHIDInput.hpp"
#include <..\External\HID\hidusage.h>
#include "Logger.hpp"
#include "Application.hpp"

using namespace EngineCore;

HIDInput::~HIDInput()
{
	Unregister();
}

bool HIDInput::Register(HWND hwnd)
{
	if (_window != 0)
	{
		SOFTBREAK;
		Unregister();
	}

    UINT devicesCount = 0;
    if (GetRawInputDeviceList(NULL, &devicesCount, sizeof(RAWINPUTDEVICELIST)) == (UINT)-1)
    {
        SENDLOG(Error, "HIDInput::Register failed, GetRawInputDeviceList failed\n");
        return false;
    }

    unique_ptr<RAWINPUTDEVICELIST[]> deviceList{new RAWINPUTDEVICELIST[devicesCount]};

    devicesCount = GetRawInputDeviceList(deviceList.get(), &devicesCount, sizeof(RAWINPUTDEVICELIST));
    if (devicesCount == (UINT)-1)
    {
        SENDLOG(Error, "HIDInput::Register failed, GetRawInputDeviceList failed\n");
        return false;
    }

    SENDLOG(Info, "Detected %u HID devices\n", devicesCount);

    for (UINT index = 0; index < devicesCount; ++index)
    {
        char name[256];
        UINT sizeofName = sizeof(name);
        if (GetRawInputDeviceInfoA(deviceList[index].hDevice, RIDI_DEVICENAME, name, &sizeofName) == (UINT)-1)
        {
            SENDLOG(Warning, "Failed to fetch HID device %u name\n", index);
            continue;
        }

        const char *deviceTypeName = "undefined";
        switch (deviceList[index].dwType)
        {
        case RIM_TYPEHID:
            deviceTypeName = "HID";
            break;
        case RIM_TYPEKEYBOARD:
            deviceTypeName = "Keyboard";
            break;
        case RIM_TYPEMOUSE:
            deviceTypeName = "Mouse";
            break;
        }

        SENDLOG(Info, "HID device %u name is %s, type is %s\n", index, name, deviceTypeName);
    }

	array<RAWINPUTDEVICE, 2> hids;

	hids[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
	hids[0].usUsage = HID_USAGE_GENERIC_MOUSE;
	hids[0].dwFlags = RIDEV_NOLEGACY;
	hids[0].hwndTarget = NULL;

	hids[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
	hids[1].usUsage = HID_USAGE_GENERIC_KEYBOARD;
	hids[1].dwFlags = RIDEV_NOLEGACY | RIDEV_APPKEYS;
	hids[1].hwndTarget = NULL;

	if (!RegisterRawInputDevices(hids.data(), (UINT)hids.size(), sizeof(RAWINPUTDEVICE)))
	{
		SENDLOG(Error, "HIDInput::Register failed, RegisterRawInputDevices failed\n");
		return false;
	}

	SENDLOG(Info, "HIDInput::Register's completed\n");
	_window = hwnd;
	return true;
}

void HIDInput::Unregister()
{
	if (_window == 0)
	{
		return;
	}

    // it's exactly the same as registering, except the flags are RIDEV_REMOVE
	array<RAWINPUTDEVICE, 2> hids;

	hids[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
	hids[0].usUsage = HID_USAGE_GENERIC_MOUSE;
	hids[0].dwFlags = RIDEV_REMOVE;
	hids[0].hwndTarget = NULL;

	hids[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
	hids[1].usUsage = HID_USAGE_GENERIC_KEYBOARD;
	hids[1].dwFlags = RIDEV_REMOVE;
	hids[1].hwndTarget = NULL;

	if (!RegisterRawInputDevices(hids.data(), (UINT)hids.size(), sizeof(RAWINPUTDEVICE)))
	{
		SENDLOG(Error, "HIDInput::Unregister failed, RegisterRawInputDevices failed\n");
		SOFTBREAK;
	}

	SENDLOG(Info, "HIDInput::Unregister's completed\n");
	_window = 0;
}

void HIDInput::Dispatch(ControlsQueue &controlsQueue, HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	RAWINPUT data;
	UINT dataSize = sizeof(data);
	if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, &data, &dataSize, sizeof(RAWINPUTHEADER)) == -1)
	{
		return;
	}
	assert(dataSize == sizeof(data));

	if (data.header.dwType == RIM_TYPEMOUSE)
	{
		const RAWMOUSE &mouse = data.data.mouse;

		if (mouse.usFlags & MOUSE_MOVE_ABSOLUTE)
		{
			HARDBREAK;
		}
		if (mouse.usFlags & MOUSE_MOVE_NOCOALESCE)
		{
			HARDBREAK;
		}

		if (mouse.lLastX || mouse.lLastY)
		{
			controlsQueue.EnqueueMouseMove(DeviceType::MouseKeyboard0, mouse.lLastX, mouse.lLastY);
		}

		if (mouse.usButtonFlags)
		{
			auto checkKey = [&controlsQueue, &mouse](USHORT windowsUpKey, USHORT windowsDownKey, vkey_t key)
			{
				if (mouse.usButtonFlags & windowsUpKey)
				{
					controlsQueue.EnqueueKey(DeviceType::MouseKeyboard0, key, ControlAction::Key::KeyStateType::Released);
				}
				else if (mouse.usButtonFlags & windowsDownKey)
				{
					controlsQueue.EnqueueKey(DeviceType::MouseKeyboard0, key, ControlAction::Key::KeyStateType::Pressed);
				}
			};

			checkKey(RI_MOUSE_BUTTON_1_UP, RI_MOUSE_BUTTON_1_DOWN, vkey_t::MButton0);
			checkKey(RI_MOUSE_BUTTON_2_UP, RI_MOUSE_BUTTON_2_DOWN, vkey_t::MButton1);
			checkKey(RI_MOUSE_BUTTON_3_UP, RI_MOUSE_BUTTON_3_DOWN, vkey_t::MButton2);
			checkKey(RI_MOUSE_BUTTON_4_UP, RI_MOUSE_BUTTON_4_DOWN, vkey_t::MButton3);
			checkKey(RI_MOUSE_BUTTON_5_UP, RI_MOUSE_BUTTON_5_DOWN, vkey_t::MButton4);

			if (mouse.usButtonFlags & RI_MOUSE_WHEEL)
			{
				//  TODO: some kind of normalization?
				controlsQueue.EnqueueMouseWheel(DeviceType::MouseKeyboard0, (SHORT)mouse.usButtonData / -WHEEL_DELTA);
			}
		}
	}
	else if (data.header.dwType == RIM_TYPEKEYBOARD)  //  TODO: fix left and right controls and alt keys
	{
		const RAWKEYBOARD &kb = data.data.keyboard;

		if (kb.VKey > 255)
		{
			SOFTBREAK;
			return;
		}

		vkey_t key = GetPlatformMapping()[kb.VKey];

        if (key == vkey_t::Shift || key == vkey_t::Control || key == vkey_t::Alt || key == vkey_t::Enter)
        {
            if (kb.Flags & RI_KEY_BREAK)
            {
                controlsQueue.EnqueueKey(DeviceType::MouseKeyboard0, key, ControlAction::Key::KeyStateType::Released);
            }
            else // making the key
            {
                controlsQueue.EnqueueKey(DeviceType::MouseKeyboard0, key, ControlAction::Key::KeyStateType::Pressed);
            }

            if (key == vkey_t::Shift)
            {
                key = (kb.MakeCode == 0x2A) ? vkey_t::LShift : vkey_t::RShift;
            }
            else if (key == vkey_t::Control)
            {
                //  TODO: left/right
            }
            else if (key == vkey_t::Alt)
            {
                //  TODO: left/right
            }
            else if (key == vkey_t::Enter)
            {
                //  TODO: left/right
            }
        }

		if (kb.Flags & RI_KEY_BREAK)
		{
			controlsQueue.EnqueueKey(DeviceType::MouseKeyboard0, key, ControlAction::Key::KeyStateType::Released);
		}
		else // making the key
		{
			controlsQueue.EnqueueKey(DeviceType::MouseKeyboard0, key, ControlAction::Key::KeyStateType::Pressed);
		}
	}
}
