#pragma once

namespace EngineCore
{
	enum class vkey_t : ui8
	{
		MButton0, MButton1, MButton2, MButton3, MButton4,
		LShift, RShift, Shift,
		LControl, RControl, Control,
		LAlt, RAlt, Alt,
		Enter,
		Space,
		Escape,
		Tab,
		PageDown, PageUp, Home, End,
		Insert,
		Delete,
		CapsLock, ScrollLock, NumLock,
		Pause,
		PrintScreen,
		Tilda,
		Backspace,
		Up, Down, Left, Right,
		Digit0, Digit1, Digit2, Digit3, Digit4, Digit5, Digit6, Digit7, Digit8, Digit9,
		A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
		F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24,
		NPad0, NPad1, NPad2, NPad3, NPad4, NPad5, NPad6, NPad7, NPad8, NPad9,
		NPadAdd, NPadSub, NPadMul, NPadDiv,
		OEM1,         //  ;: for US
		OEM2,         //  /? for US
		OEM3,         //  `~ for US
		OEM4,         //  [{ for US
		OEM5,         //  \| for US
		OEM6,         //  ]} for US
		OEM7,         //  '" for US
		OEMAdd,       //  + any country
		OEMComma,     //  , any country
		OEMSub,       //  - any country
		OEMDot,       //  . any country
		Select, Start, Stop,
		L1, L2, L3, L4, R1, R2, R3, R4,
		Key0, Key1, Key2, Key3, Key4, Key5, Key6, Key7, Key8, Key9,
		Key10, Key11, Key12, Key13, Key14, Key15,
		Undefined,
		_size
	};

	inline bool operator == (vkey_t vKey, char pKey) { return vKey == vkey_t(pKey); }
	inline bool operator == (char pKey, vkey_t vKey) { return vKey == vkey_t(pKey); }
	inline bool operator != (vkey_t vKey, char pKey) { return vKey != vkey_t(pKey); }
	inline bool operator != (char pKey, vkey_t vKey) { return vKey != vkey_t(pKey); }

	inline bool operator == (vkey_t vKey, ui8 pKey) { return vKey == vkey_t(pKey); }
	inline bool operator == (ui8 pKey, vkey_t vKey) { return vKey == vkey_t(pKey); }
	inline bool operator != (vkey_t vKey, ui8 pKey) { return vKey != vkey_t(pKey); }
	inline bool operator != (ui8 pKey, vkey_t vKey) { return vKey != vkey_t(pKey); }

	auto GetPlatformMapping() -> const array<vkey_t, 256> &;
}