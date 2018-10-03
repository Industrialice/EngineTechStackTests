#include "BasicHeader.hpp"
#include "VirtualKeys.hpp"

using namespace EngineCore;

bool VirtualKeys::IsLetter(KeyCode key)
{
    return key >= KeyCode::A && key <= KeyCode::Z;
}

bool VirtualKeys::IsDigit(KeyCode key)
{
    return key >= KeyCode::Digit0 && key <= KeyCode::Digit9;
}

bool VirtualKeys::IsNPadKey(KeyCode key)
{
    return key >= KeyCode::NPad0 && key <= KeyCode::NPadDiv;
}

bool VirtualKeys::IsNPadDigit(KeyCode key)
{
    return key >= KeyCode::NPad0 && key <= KeyCode::NPad9;
}

bool VirtualKeys::IsNPadArrow(KeyCode key)
{
    return key == KeyCode::NPad4 || key == KeyCode::NPad8 || key == KeyCode::NPad2 || key == KeyCode::NPad6;
}

bool VirtualKeys::IsMouseButton(KeyCode key)
{
    return key >= KeyCode::MButton0 && key <= KeyCode::MButton6;
}

bool VirtualKeys::IsArrowKey(KeyCode key)
{
    return key >= KeyCode::UpArrow && key <= KeyCode::RightArrow;
}

bool VirtualKeys::IsFKey(KeyCode key)
{
    return key >= KeyCode::F1 && key <= KeyCode::F24;
}

bool VirtualKeys::IsShift(KeyCode key)
{
    return key == KeyCode::LShift || key == KeyCode::RShift;
}

bool VirtualKeys::IsControl(KeyCode key)
{
    return key == KeyCode::LControl || key == KeyCode::RControl;
}

bool VirtualKeys::IsAlt(KeyCode key)
{
    return key == KeyCode::LAlt || key == KeyCode::RAlt;
}

bool VirtualKeys::IsSystem(KeyCode key)
{
    return key == KeyCode::LSystem || key == KeyCode::RSystem;
}

bool VirtualKeys::IsEnter(KeyCode key)
{
    return key == KeyCode::LEnter || key == KeyCode::REnter;
}

bool VirtualKeys::IsDelete(KeyCode key)
{
    return key == KeyCode::LDelete || key == KeyCode::RDelete;
}

ui32 VirtualKeys::KeyNumber(KeyCode key)
{
    if (IsDigit(key))
    {
        return (ui32)key - (ui32)KeyCode::Digit0;
    }
    if (IsMouseButton(key))
    {
        return (ui32)key - (ui32)KeyCode::MButton0;
    }
    if (IsNPadDigit(key))
    {
        return (ui32)key - (ui32)KeyCode::NPad0;
    }
    if (IsFKey(key))
    {
        return (ui32)key - (ui32)KeyCode::F1 + 1;
    }
    return 0;
}

char VirtualKeys::ToAlpha(KeyCode key, bool isUpperCase)
{
    if (IsLetter(key))
    {
        return ((ui32)key - (ui32)KeyCode::A) + (isUpperCase ? 'A' : 'a');
    }
    return '\0';
}
