#pragma once

namespace EngineCore
{
    enum class vkeyt : ui8
    {
        Undefined,
        MButton0, MButton1, MButton2, MButton3, MButton4, MButton5, MButton6,
        LShift, RShift,
        LControl, RControl,
        LAlt, RAlt,
        LSystem, RSystem,
        LEnter, REnter,
        LDelete, RDelete,
        Space,
        Escape,
        Tab,
        PageDown, PageUp,
        Home, End,
        Insert,
        CapsLock, ScrollLock, NumLock,
        Pause,
        PrintScreen,
        Tilda,
        Backspace,
        UpArrow, DownArrow, LeftArrow, RightArrow,
        Digit0, Digit1, Digit2, Digit3, Digit4, Digit5, Digit6, Digit7, Digit8, Digit9,
        A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
        F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24,
        NPad0, NPad1, NPad2, NPad3, NPad4, NPad5, NPad6, NPad7, NPad8, NPad9,
        NPadPlus, NPadMinus, NPadMul, NPadDiv,
        Plus, Minus,
        Comma,
        Dot,
        OEM1,         //  ;: for US
        OEM2,         //  /? for US
        OEM3,         //  `~ for US
        OEM4,         //  [{ for US
        OEM5,         //  \| for US
        OEM6,         //  ]} for US
        OEM7,         //  '" for US
        Select, Start, Stop,
        L1, L2, L3, L4, R1, R2, R3, R4,
        Key0, Key1, Key2, Key3, Key4, Key5, Key6, Key7, Key8, Key9,
        Key10, Key11, Key12, Key13, Key14, Key15,
        _size
    };

    /*inline bool operator == (vkeyt vKey, char pKey) { return vKey == vkeyt(pKey); }
    inline bool operator == (char pKey, vkeyt vKey) { return vKey == vkeyt(pKey); }
    inline bool operator != (vkeyt vKey, char pKey) { return vKey != vkeyt(pKey); }
    inline bool operator != (char pKey, vkeyt vKey) { return vKey != vkeyt(pKey); }

    inline bool operator == (vkeyt vKey, ui8 pKey) { return vKey == vkeyt(pKey); }
    inline bool operator == (ui8 pKey, vkeyt vKey) { return vKey == vkeyt(pKey); }
    inline bool operator != (vkeyt vKey, ui8 pKey) { return vKey != vkeyt(pKey); }
    inline bool operator != (ui8 pKey, vkeyt vKey) { return vKey != vkeyt(pKey); }*/

    namespace VirtualKeys
    {
        bool IsLetter(vkeyt key);
        bool IsDigit(vkeyt key);
        bool IsNPadKey(vkeyt key);
        bool IsNPadDigit(vkeyt key);
        bool IsNPadArrow(vkeyt key);
        bool IsMouseButton(vkeyt key);
        bool IsArrowKey(vkeyt key);
        bool IsFKey(vkeyt key);
        bool IsShift(vkeyt key); // either LShift or RShift
        bool IsControl(vkeyt key); // either LControl or RControl
        bool IsAlt(vkeyt key); // either LAlt or RAlt
        bool IsSystem(vkeyt key); // either LSystem or RSystem
        bool IsEnter(vkeyt key); // either LEnter or REnter
        bool IsDelete(vkeyt key); // either LDelete or RDelete
        ui32 KeyNumber(vkeyt key); // accepts key names that end with a number, returns 0 in other cases, returns 1 for vkeyt::F1
        char ToAlpha(vkeyt key, bool isUpperCase = true); // accepts A-Z keys, returns '\0' if key is not a letter
    }
}