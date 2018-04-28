#include "BasicHeader.hpp"
#include "VirtualKeys.hpp"

using namespace EngineCore;

bool VirtualKeys::IsLetter(vkeyt key)
{
    return key >= vkeyt::A && key <= vkeyt::Z;
}

bool VirtualKeys::IsDigit(vkeyt key)
{
    return key >= vkeyt::Digit0 && key <= vkeyt::Digit9;
}

bool VirtualKeys::IsNPadKey(vkeyt key)
{
    return key >= vkeyt::NPad0 && key <= vkeyt::NPadDiv;
}

bool VirtualKeys::IsNPadDigit(vkeyt key)
{
    return key >= vkeyt::NPad0 && key <= vkeyt::NPad9;
}

bool VirtualKeys::IsNPadArrow(vkeyt key)
{
    return key == vkeyt::NPad4 || key == vkeyt::NPad8 || key == vkeyt::NPad2 || key == vkeyt::NPad6;
}

bool VirtualKeys::IsMouseButton(vkeyt key)
{
    return key >= vkeyt::MButton0 && key <= vkeyt::MButton6;
}

bool VirtualKeys::IsArrowKey(vkeyt key)
{
    return key >= vkeyt::UpArrow && key <= vkeyt::RightArrow;
}

bool VirtualKeys::IsFKey(vkeyt key)
{
    return key >= vkeyt::F1 && key <= vkeyt::F24;
}

bool VirtualKeys::IsShift(vkeyt key)
{
    return key == vkeyt::LShift || key == vkeyt::RShift;
}

bool VirtualKeys::IsControl(vkeyt key)
{
    return key == vkeyt::LControl || key == vkeyt::RControl;
}

bool VirtualKeys::IsAlt(vkeyt key)
{
    return key == vkeyt::LAlt || key == vkeyt::RAlt;
}

bool VirtualKeys::IsSystem(vkeyt key)
{
    return key == vkeyt::LSystem || key == vkeyt::RSystem;
}

bool VirtualKeys::IsEnter(vkeyt key)
{
    return key == vkeyt::LEnter || key == vkeyt::REnter;
}

bool VirtualKeys::IsDelete(vkeyt key)
{
    return key == vkeyt::LDelete || key == vkeyt::RDelete;
}

ui32 VirtualKeys::KeyNumber(vkeyt key)
{
    if (IsDigit(key))
    {
        return (ui32)key - (ui32)vkeyt::Digit0;
    }
    if (IsMouseButton(key))
    {
        return (ui32)key - (ui32)vkeyt::MButton0;
    }
    if (IsNPadDigit(key))
    {
        return (ui32)key - (ui32)vkeyt::NPad0;
    }
    if (IsFKey(key))
    {
        return (ui32)key - (ui32)vkeyt::F1 + 1;
    }
    return 0;
}

char VirtualKeys::ToAlpha(vkeyt key, bool isUpperCase)
{
    if (IsLetter(key))
    {
        return ((ui32)key - (ui32)vkeyt::A) + (isUpperCase ? 'A' : 'a');
    }
    return '\0';
}
