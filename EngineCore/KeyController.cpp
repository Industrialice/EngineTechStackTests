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
    _actions.push_back({key, keyState, TimeMoment::Now(), deviceType});
}

void ControlsQueue::EnqueueMouseMove(DeviceType deviceType, i32 deltaX, i32 deltaY)
{
    _actions.push_back({deltaX, deltaY, TimeMoment::Now(), deviceType});
}

void ControlsQueue::EnqueueMouseWheel(DeviceType deviceType, i32 delta)
{
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

KeyController::~KeyController()
{
    if (_mutableListeners.empty() == false)
    {
        SENDLOG(Warning, "KeyController is being deleted, but it still has listeners\n");
    }
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
    if (_isMutableListenersDirty)
    {
        _immutableListeners.clear();

        for (auto &value : _mutableListeners)
        {
            _immutableListeners.push_back({value.listener, value.deviceMask, &value});
        }

        _isMutableListenersDirty = false;
    }

    _isCurrentlyEnumeratingImmutableListeners = true;

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

    for (const auto &listener : _immutableListeners)
    {
        if ((listener.deviceMask + action.deviceType) == listener.deviceMask)
        {
            listener.listener(cookedAction);
        }
    }

    _isCurrentlyEnumeratingImmutableListeners = false;
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
        return ListenerHandle();
    }

    _isMutableListenersDirty = true;

    _mutableListeners.push_front({callback, deviceMask});

    return ListenerHandle{shared_from_this(), &_mutableListeners.front()};
}

void KeyController::RemoveListener(ListenerHandle &handle)
{
    const auto &strongOwner = handle._owner.lock();
    if (strongOwner == nullptr)
    {
        return;
    }

    if (strongOwner.owner_before(shared_from_this()) != false || shared_from_this().owner_before(strongOwner) != false)
    {
        SOFTBREAK;
        return;
    }

    _isMutableListenersDirty = true;

    _mutableListeners.remove_if([&handle](const ListenerAndMask &value) { return &value == handle._listenerAndMask; });

    if (_isCurrentlyEnumeratingImmutableListeners) // we need to make sure that the removed listener won't be called
    {
        auto result = std::find_if(_immutableListeners.begin(), _immutableListeners.end(),
            [&handle](const ImmutableListener &value) { return value.listenerAndMask == handle._listenerAndMask; });

        if (result != _immutableListeners.end())
        {
            result->deviceMask = DeviceType_None; // make sure it won't be called
        }
    }

    handle._owner.reset();
}

auto KeyController::GetKeyInfo(vkeyt key, DeviceType deviceType) const -> KeyInfo
{
    ui32 deviceIndex = Funcs::IndexOfMostSignificantNonZeroBit((ui32)deviceType);
    auto &deviceKeyStates = _keyStates[deviceIndex];
    return deviceKeyStates[(size_t)key];
}

void KeyController::ListenerHandle::Remove()
{
    const auto &strongOwner = _owner.lock();
    if (strongOwner != nullptr)
    {
        strongOwner->RemoveListener(*this);
    }
}

KeyController::ListenerHandle::~ListenerHandle()
{
    Remove();
}

auto KeyController::ListenerHandle::operator=(ListenerHandle &&source) -> ListenerHandle &
{
    assert(this != &source);
    Remove();
    _owner = move(source._owner);
    _listenerAndMask = move(source._listenerAndMask);
    return *this;
}

bool KeyController::ListenerHandle::operator == (const ListenerHandle &other) const
{
    return (_owner.owner_before(other._owner) == false && other._owner.owner_before(_owner) == false) && _listenerAndMask == other._listenerAndMask;
}

bool KeyController::ListenerHandle::operator != (const ListenerHandle &other) const
{
    return !(this->operator==(other));
}

bool KeyController::KeyInfo::IsPressed() const
{
    return keyState != KeyStateType::Released;
}
