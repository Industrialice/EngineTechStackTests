#include "BasicHeader.hpp"
#include "KeyController.hpp"
#include "Logger.hpp"
#include "Application.hpp"

using namespace EngineCore;

shared_ptr<KeyController> KeyController::New()
{
    struct Proxy : public KeyController
    {
        Proxy() : KeyController() {}
    };
    return make_shared<Proxy>();
}

void KeyController::Dispatch(const ControlAction &action)
{
    if (_isDispatchingInProgress)
    {
        SOFTBREAK;
        return;
    }

    auto cookedAction = action;
    ui32 value = cookedAction.device._value;

    if (auto mouseMoveAction = std::get_if<ControlAction::MouseMove>(&cookedAction.action))
    {
        if (_mousePositionInfos[0])
        {
            *_mousePositionInfos[0] += mouseMoveAction->delta;
        }
        else
        {
            _mousePositionInfos[0] = mouseMoveAction->delta;
        }
    }
    else if (auto keyAction = std::get_if<ControlAction::Key>(&cookedAction.action))
    {
        auto getKeyStates = [this, value]() -> AllKeyStates &
        {
            if (value == DeviceType::MouseKeyboard)
            {
                return _mouseKeyboardKeyStates[0];
            }
            ui32 index = Funcs::IndexOfMostSignificantNonZeroBit(value) - Funcs::IndexOfMostSignificantNonZeroBit(DeviceType::Joystick0);
            return _joystickKeyStates[index];
        };

        auto &keyInfo = getKeyStates()[(size_t)keyAction->key];

        bool wasPressed = keyInfo.keyState == KeyInfo::KeyState::Pressed;
        bool isRepeating = wasPressed && keyAction->keyState == KeyInfo::KeyState::Pressed;

        ui32 currentKSCTimes = keyInfo.timesKeyStateChanged;
        if (keyInfo.keyState != keyAction->keyState || isRepeating)
        {
            ++currentKSCTimes;
        }

        keyInfo = {keyAction->keyState, currentKSCTimes, cookedAction.occuredAt};

        if (isRepeating)
        {
            keyAction->keyState = KeyInfo::KeyState::Repeated;
        }
    }
    else if (auto mouseSetPositionAction = std::get_if<ControlAction::MouseSetPosition>(&cookedAction.action))
    {
        _mousePositionInfos[0] = mouseSetPositionAction->position;
    }
    else if (auto touchDownAction = std::get_if<ControlAction::TouchDown>(&cookedAction.action))
    {
        ui32 deviceIndex = Funcs::IndexOfMostSignificantNonZeroBit(value) - Funcs::IndexOfMostSignificantNonZeroBit(DeviceType::Touch0);
        _touchPositionInfos[deviceIndex] = touchDownAction->position;
    }
    else if (auto touchMoveAction = std::get_if<ControlAction::TouchMove>(&cookedAction.action))
    {
        ui32 deviceIndex = Funcs::IndexOfMostSignificantNonZeroBit(value) - Funcs::IndexOfMostSignificantNonZeroBit(DeviceType::Touch0);
        if (!_touchPositionInfos[deviceIndex])
        {
            SOFTBREAK;
            return;
        }
        *_touchPositionInfos[deviceIndex] += touchMoveAction->delta;
    }
    else if (auto touchUpAction = std::get_if<ControlAction::TouchUp>(&cookedAction.action))
    {
        ui32 deviceIndex = Funcs::IndexOfMostSignificantNonZeroBit(value) - Funcs::IndexOfMostSignificantNonZeroBit(DeviceType::Touch0);
        if (!_touchPositionInfos[deviceIndex])
        {
            SOFTBREAK;
            return;
        }
        _touchPositionInfos[deviceIndex] = {};
    }

    _isDispatchingInProgress = true;
    for (i32 index = (i32)_listeners.size() - 1; index >= 0; --index)
    {
        const auto &listener = _listeners[index];
        if ((listener.deviceMask + action.device) == listener.deviceMask)
        {
            if (listener.listener(cookedAction))
            {
                break;
            }
        }
    }
    _isDispatchingInProgress = false;

    if (_isListenersDirty)
    {
        _listeners.erase(std::remove_if(_listeners.begin(), _listeners.end(), [](const MessageListener &listener) { return listener.deviceMask == DeviceType::_None; }));
        _isListenersDirty = false;
    }
}

void KeyController::Dispatch(std::experimental::generator<ControlAction> enumerable)
{
    for (const auto &action : enumerable)
    {
        Dispatch(action);
    }
}

void KeyController::Update()
{}

auto KeyController::OnControlAction(const ListenerCallbackType &callback, DeviceType deviceMask) -> ListenerHandle
{
    if (deviceMask == DeviceType::_None) // not an error, but probably not what you wanted either
    {
        SOFTBREAK;
        return {};
    }

    ui32 id = AssignId<MessageListener, ui32, &MessageListener::id>(_currentId, _listeners.begin(), _listeners.end());
    _listeners.push_back({callback, deviceMask, id});
    return {shared_from_this(), id};
}

void KeyController::RemoveListener(ListenerHandle &handle)
{
    if (handle.Owner().expired())
    {
        return;
    }

    ASSUME(Funcs::AreSharedPointersEqual(handle.Owner(), shared_from_this()));

    uiw index = 0;
    for (; ; ++index)
    {
        if (_listeners[index].id == handle.Id())
        {
            break;
        }
    }

    if (_isDispatchingInProgress)
    {
        _listeners[index].deviceMask = DeviceType::_None;
        _isListenersDirty = true;
    }
    else
    {
        _listeners.erase(_listeners.begin() + index);
    }

    handle.Owner().reset();
}

auto KeyController::GetKeyInfo(vkeyt key, DeviceType device) const -> KeyInfo
{
    ASSUME(Funcs::IndexOfMostSignificantNonZeroBit(device._value) == Funcs::IndexOfLeastSignificantNonZeroBit(device._value));
    ui32 value = device._value;
    if (value == DeviceType::MouseKeyboard)
    {
        return _mouseKeyboardKeyStates[0][(ui32)key];
    }
    if (value >= DeviceType::Joystick0 && value <= DeviceType::Joystick7)
    {
        ui32 index = Funcs::IndexOfMostSignificantNonZeroBit(value) - Funcs::IndexOfMostSignificantNonZeroBit(DeviceType::Joystick0);
        return _joystickKeyStates[index][(ui32)key];
    }
    return {};
}

auto KeyController::GetPositionInfo(DeviceType device) const -> optional<i32Vector2>
{
    ASSUME(Funcs::IndexOfMostSignificantNonZeroBit(device._value) == Funcs::IndexOfLeastSignificantNonZeroBit(device._value));
    ui32 value = device._value;
    if (value == DeviceType::MouseKeyboard)
    {
        return _mousePositionInfos[0];
    }
    if (value >= DeviceType::Touch0 && value <= DeviceType::Touch9)
    {
        ui32 index = Funcs::IndexOfMostSignificantNonZeroBit(value) - Funcs::IndexOfMostSignificantNonZeroBit(DeviceType::Touch0);
        return _touchPositionInfos[index];
    }
    return {};
}

auto KeyController::GetAllKeyStates(DeviceType device) const -> const AllKeyStates &
{
    ASSUME(Funcs::IndexOfMostSignificantNonZeroBit(device._value) == Funcs::IndexOfLeastSignificantNonZeroBit(device._value));
    ui32 value = device._value;
    if (value == DeviceType::MouseKeyboard)
    {
        return _mouseKeyboardKeyStates[0];
    }
    if (value >= DeviceType::Joystick0 && value <= DeviceType::Joystick7)
    {
        ui32 index = Funcs::IndexOfMostSignificantNonZeroBit(value) - Funcs::IndexOfMostSignificantNonZeroBit(DeviceType::Joystick0);
        return _joystickKeyStates[index];
    }
    return _defaultKeyStates;
}
