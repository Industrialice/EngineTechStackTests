#pragma once

#include "VirtualKeys.hpp"
#include "RingBuffer.h"
#include "ListenerHandle.hpp"

namespace EngineCore
{
    ENUM_COMBINABLE(DeviceType, ui32,
        _None = 0,
        MouseKeyboard = Funcs::BitPos(0),
        Touch0 = Funcs::BitPos(1),
        Touch1 = Funcs::BitPos(2),
        Touch2 = Funcs::BitPos(3),
        Touch3 = Funcs::BitPos(4),
        Touch4 = Funcs::BitPos(5),
        Touch5 = Funcs::BitPos(6),
        Touch6 = Funcs::BitPos(7),
        Touch7 = Funcs::BitPos(8),
        Touch8 = Funcs::BitPos(9),
        Touch9 = Funcs::BitPos(10),
        Joystick0 = Funcs::BitPos(11),
        Joystick1 = Funcs::BitPos(12),
        Joystick2 = Funcs::BitPos(13),
        Joystick3 = Funcs::BitPos(14),
        Joystick4 = Funcs::BitPos(15),
        Joystick5 = Funcs::BitPos(16),
        Joystick6 = Funcs::BitPos(17),
        Joystick7 = Funcs::BitPos(18),
        _AllTouches = Touch0 | Touch1 | Touch2 | Touch3 | Touch4 | Touch5 | Touch6 | Touch7 | Touch8 | Touch9,
        _AllJoysticks = Joystick0 | Joystick1 | Joystick2 | Joystick3 | Joystick4 | Joystick5 | Joystick6 | Joystick7,
        _AllDevices = MouseKeyboard | _AllTouches | _AllJoysticks);

    inline ui32 DeviceIndex(DeviceType device)
    {
        ASSUME(Funcs::IndexOfMostSignificantNonZeroBit(device._value) == Funcs::IndexOfLeastSignificantNonZeroBit(device._value));
        ui32 value = device._value;
        if (value >= DeviceType::Touch0 && value <= DeviceType::Touch9)
        {
            return Funcs::IndexOfMostSignificantNonZeroBit(value) - Funcs::IndexOfMostSignificantNonZeroBit(DeviceType::Touch0);
        }
        if (value >= DeviceType::Joystick0 && value <= DeviceType::Joystick7)
        {
            return Funcs::IndexOfMostSignificantNonZeroBit(value) - Funcs::IndexOfMostSignificantNonZeroBit(DeviceType::Joystick0);
        }
        return 0;
    }

    struct ControlAction
    {
        struct Key
        {
            KeyCode key{};
            enum class KeyState { Released, Pressed, Repeated } keyState{};
        };
        struct MouseSetPosition
        {
            i32Vector2 position{};
        };
        struct MouseMove
        {
            i32Vector2 delta{};
        };
        struct MouseWheel
        {
            i32 delta{};
        };
        struct TouchDown
        {
            i32Vector2 position{};
        };
        struct TouchMove
        {
            i32Vector2 delta{};
        };
        struct TouchUp
        {
            i32Vector2 lastPosition{};
        };
        struct TouchLongPress
        {
            i32Vector2 position{};
        };
        struct TouchDoubleTap
        {
            i32Vector2 position{};
        };
        struct TouchZoomStart
        {
            i32Vector2 focusPoint{};
        };
        struct TouchZoom
        {
            i32Vector2 focusPoint{};
            f32 delta{};
        };
        struct TouchZoomEnd
        {};

        variant<Key, MouseSetPosition, MouseMove, MouseWheel, TouchDown, TouchMove, TouchUp, TouchLongPress, TouchDoubleTap, TouchZoomStart, TouchZoom, TouchZoomEnd> action{};
        TimeMoment occuredAt{};
        DeviceType device = DeviceType::_None;

        ControlAction() {}
        ControlAction(Key actionData, const TimeMoment &occuredAt, DeviceType device) : action{actionData}, occuredAt{occuredAt}, device{device} {}
        ControlAction(MouseSetPosition actionData, const TimeMoment &occuredAt, DeviceType device) : action{actionData}, occuredAt{occuredAt}, device{device} {}
        ControlAction(MouseMove actionData, const TimeMoment &occuredAt, DeviceType device) : action{actionData}, occuredAt{occuredAt}, device{device} {}
        ControlAction(MouseWheel actionData, const TimeMoment &occuredAt, DeviceType device) : action{actionData}, occuredAt{occuredAt}, device{device} {}
        ControlAction(TouchDown actionData, const TimeMoment &occuredAt, DeviceType device) : action{actionData}, occuredAt{occuredAt}, device{device} {}
        ControlAction(TouchMove actionData, const TimeMoment &occuredAt, DeviceType device) : action{actionData}, occuredAt{occuredAt}, device{device} {}
        ControlAction(TouchUp actionData, const TimeMoment &occuredAt, DeviceType device) : action{actionData}, occuredAt{occuredAt}, device{device} {}
        ControlAction(TouchLongPress actionData, const TimeMoment &occuredAt, DeviceType device) : action{actionData}, occuredAt{occuredAt}, device{device} {}
        ControlAction(TouchDoubleTap actionData, const TimeMoment &occuredAt, DeviceType device) : action{actionData}, occuredAt{occuredAt}, device{device} {}
        ControlAction(TouchZoomStart actionData, const TimeMoment &occuredAt, DeviceType device) : action{actionData}, occuredAt{occuredAt}, device{device} {}
        ControlAction(TouchZoom actionData, const TimeMoment &occuredAt, DeviceType device) : action{actionData}, occuredAt{occuredAt}, device{device} {}
        ControlAction(TouchZoomEnd actionData, const TimeMoment &occuredAt, DeviceType device) : action{actionData}, occuredAt{occuredAt}, device{device} {}
    };

    class ControlsQueue
    {
        RingBuffer<ControlAction, 256> _actions;

    public:

        void clear()
        {
            _actions.clear();
        }

        uiw size() const
        {
            return _actions.size();
        }

        template <typename T> void Enqueue(DeviceType device, T action)
        {
            ASSUME(device != DeviceType::_None);
            _actions.push_back(ControlAction{action, TimeMoment::Now(), device});
        }

        std::experimental::generator<ControlAction> Enumerate() const
        {
            for (const auto &action : _actions)
            {
                co_yield action;
            }
        }
    };

    class IKeyController
    {
        static void RemoveListener(IKeyController *instance, void *handle);

    public:
        using ListenerCallbackType = function<bool(const ControlAction &action)>; // return true if the action needs to be blocked from going to any subsequent listeners
        using ListenerHandle = TListenerHandle<IKeyController, ui32, RemoveListener>;

        struct KeyInfo
        {
            using KeyState = ControlAction::Key::KeyState;
            KeyState keyState{};
            ui32 timesKeyStateChanged{};
            TimeMoment occuredAt = TimeMoment::Now();

            bool IsPressed() const
            {
                return keyState != KeyState::Released;
            }
        };

        using AllKeyStates = array<KeyInfo, (size_t)KeyCode::_size>;

        virtual ~IKeyController() = default;
        virtual void Dispatch(const ControlAction &action) = 0;
        virtual void Dispatch(std::experimental::generator<ControlAction> enumerable) = 0;
        virtual void Update() = 0; // may be used for key repeating
        [[nodiscard]] virtual KeyInfo GetKeyInfo(KeyCode key, DeviceType device = DeviceType::MouseKeyboard) const = 0; // always default for Touch
        [[nodiscard]] virtual optional<i32Vector2> GetPositionInfo(DeviceType device = DeviceType::MouseKeyboard) const = 0; // always nullopt for Joystick
        [[nodiscard]] virtual const AllKeyStates &GetAllKeyStates(DeviceType device = DeviceType::MouseKeyboard) const = 0; // always default for Touch
        [[nodiscard]] virtual ListenerHandle OnControlAction(const ListenerCallbackType &callback, DeviceType deviceMask) = 0;
        virtual void RemoveListener(ListenerHandle &handle) = 0;
    };

    class EmptyKeyController : public IKeyController
    {
    protected:
        EmptyKeyController() = default;
        EmptyKeyController(EmptyKeyController &&) = delete;
        EmptyKeyController &operator = (EmptyKeyController &&) = delete;

    public:
        static shared_ptr<EmptyKeyController> New()
        {
            struct Proxy : public EmptyKeyController
            {
                Proxy() : EmptyKeyController() {}
            };
            return make_shared<Proxy>();
        }

        virtual ~EmptyKeyController() override = default;
        virtual void Dispatch(const ControlAction &action) override {}
        virtual void Dispatch(std::experimental::generator<ControlAction> enumerable) override {}
        virtual void Update() override {}
        [[nodiscard]] virtual KeyInfo GetKeyInfo(KeyCode key, DeviceType device = DeviceType::MouseKeyboard) const override { return {}; }
        [[nodiscard]] virtual optional<i32Vector2> GetPositionInfo(DeviceType device = DeviceType::MouseKeyboard) const override { return {}; }
        [[nodiscard]] virtual const AllKeyStates &GetAllKeyStates(DeviceType device = DeviceType::MouseKeyboard) const override { return _defaultKeyStates; }
        [[nodiscard]] virtual ListenerHandle OnControlAction(const ListenerCallbackType &callback, DeviceType deviceMask) override { return {}; }
        virtual void RemoveListener(ListenerHandle &handle) override {}

    private:
        const AllKeyStates _defaultKeyStates{};
    };
}