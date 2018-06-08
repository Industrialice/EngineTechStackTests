#include "BasicHeader.hpp"
#include "KeyController.hpp"
#include "Logger.hpp"
#include "Application.hpp"

using namespace EngineCore;

void ControlsQueue::clear()
{
    _actions.clear();
}

uiw ControlsQueue::size() const
{
    return _actions.size();
}

void ControlsQueue::EnqueueKey(DeviceType deviceType, vkeyt key, ControlAction::Key::KeyStateType keyState)
{
    ASSUME(deviceType != DeviceType_None);
    _actions.push_back({key, keyState, TimeMoment::Now(), deviceType});
}

void ControlsQueue::EnqueueMouseMove(DeviceType deviceType, i32 deltaX, i32 deltaY)
{
    ASSUME(deviceType != DeviceType_None);
    _actions.push_back({deltaX, deltaY, TimeMoment::Now(), deviceType});
}

void ControlsQueue::EnqueueMouseWheel(DeviceType deviceType, i32 delta)
{
    ASSUME(deviceType != DeviceType_None);
    _actions.push_back({delta, TimeMoment::Now(), deviceType});
}

auto ControlsQueue::Enumerate() const -> std::experimental::generator<ControlAction>
{
    for (const auto &action : _actions)
    {
        co_yield action;
    }
}

shared_ptr<KeyController> KeyController::New()
{
    struct Proxy : public KeyController
    {
        Proxy() : KeyController() {}
    };
    return make_shared<Proxy>();
}

KeyController::KeyController()
{
    for (auto &deviceKeyStates : _keyStates)
    {
        for (auto &keyState : deviceKeyStates)
        {
            keyState = {KeyInfo::KeyStateType::Released, 0, TimeMoment::Now()};
        }
    }
}

void KeyController::Dispatch(const ControlAction &action)
{
    if (_isDispatchingInProgress)
    {
        SOFTBREAK;
        return;
    }

    auto cookedAction = action;

    if (auto keyAction = std::get_if<ControlAction::Key>(&cookedAction.action))
    {
        ui32 deviceIndex = Funcs::IndexOfLeastSignificantNonZeroBit((ui32)cookedAction.deviceType);
        auto &deviceKeyStates = _keyStates[deviceIndex];
        auto &keyInfo = deviceKeyStates[(size_t)keyAction->key];

        bool wasPressed = keyInfo.keyState == KeyInfo::KeyStateType::Pressed;
        bool isRepeating = wasPressed && keyAction->keyState == KeyInfo::KeyStateType::Pressed;

        ui32 currentKSCTimes = keyInfo.timesKeyStateChanged;
        if (keyInfo.keyState != keyAction->keyState || isRepeating)
        {
            ++currentKSCTimes;
        }

        keyInfo = {keyAction->keyState, currentKSCTimes, cookedAction.occuredAt};

        if (isRepeating)
        {
            keyAction->keyState = KeyInfo::KeyStateType::Repeated;
        }
    }

    _isDispatchingInProgress = true;
    for (i32 index = (i32)_listeners.size() - 1; index >= 0; --index)
    {
        const auto &listener = _listeners[index];
        if ((listener.deviceMask + action.deviceType) == listener.deviceMask)
        {
            listener.listener(cookedAction);
        }
    }
    _isDispatchingInProgress = false;

    if (_isListenersDirty)
    {
        _listeners.erase(std::remove_if(_listeners.begin(), _listeners.end(), [](const MessageListener &listener) { return listener.deviceMask == DeviceType_None; }));
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

auto KeyController::AddListener(const ListenerCallbackType &callback, DeviceType deviceMask) -> ListenerHandle
{
    if (deviceMask == DeviceType_None) // not an error, but probably not what you wanted either
    {
        SOFTBREAK;
        return {};
    }

    ui32 id;
    if (_currentId != ui32_max)
    {
        id = _currentId;
        ++_currentId;
    }
    else
    {
        id = FindIDForListener();
    }

    _listeners.push_back({callback, deviceMask, id});

    return {shared_from_this(), id};
}

void KeyController::RemoveListener(ListenerHandle &handle)
{
    if (handle._owner.expired())
    {
        return;
    }

    ASSUME(Funcs::AreSharedPointersEqual(handle._owner, shared_from_this()));

    uiw index = 0;
    for (; ; ++index)
    {
        ASSUME(index < _listeners.size());
        if (_listeners[index].id == handle._id)
        {
            break;
        }
    }

    if (_isDispatchingInProgress)
    {
        _listeners[index].deviceMask = DeviceType_None;
        _isListenersDirty = true;
    }
    else
    {
        _listeners.erase(_listeners.begin() + index);
    }

    handle._owner.reset();
}

NOINLINE ui32 KeyController::FindIDForListener() const
{
    // this should never happen unless you have bogus code that calls AddListener/RemoveListener repeatedly
    // you'd need 50k AddListener calls per second to exhaust ui32 within 24 hours
    SOFTBREAK;
    return FindSmallestID<MessageListener, ui32, &MessageListener::id>(_listeners.begin(), _listeners.end());
}

auto KeyController::GetKeyInfo(vkeyt key, DeviceType deviceType) const -> KeyInfo
{
    ASSUME(deviceType != DeviceType_None);
    ui32 deviceIndex = Funcs::IndexOfMostSignificantNonZeroBit((ui32)deviceType);
    auto &deviceKeyStates = _keyStates[deviceIndex];
    return deviceKeyStates[(size_t)key];
}

bool KeyController::KeyInfo::IsPressed() const
{
    return keyState != KeyStateType::Released;
}
