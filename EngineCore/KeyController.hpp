#pragma once

#include "VirtualKeys.hpp"
#include "RingBuffer.h"
#include "ListenerHandle.hpp"

// TODO: window went active/inactive, multiple windows handling

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
        _AllTouchs = Touch0 | Touch1 | Touch2 | Touch3 | Touch4 | Touch5 | Touch6 | Touch7 | Touch8 | Touch9,
        _AllJoysticks = Joystick0 | Joystick1 | Joystick2 | Joystick3 | Joystick4 | Joystick5 | Joystick6 | Joystick7,
        _AllDevices = MouseKeyboard | _AllTouchs | _AllJoysticks);

    ui32 DeviceIndex(DeviceType device);

	struct ControlAction
	{
		struct Key
		{
            vkeyt key{};
            enum class KeyStateType { Released, Pressed, Repeated } keyState{};
		};
        struct MouseSetPosition
        {
            i32 x{}, y{};
        };
		struct MouseMove
		{
            i32 deltaX{}, deltaY{};
		};
		struct MouseWheel
		{
            i32 delta{};
		};
        struct TouchDown
        {
            i32 x{}, y{};
        };
        struct TouchMove
        {
            i32 deltaX{}, deltaY{};
        };
        struct TouchUp
        {
            i32 lastX{}, lastY{};
        };
        struct TouchLongPress
        {
            i32 x{}, y{};
        };
        struct TouchDoubleTap
        {
            i32 x{}, y{};
        };
        struct TouchZoomStart
        {
            i32 focusX{}, focusY{};
        };
        struct TouchZoom
        {
            i32 focusX{}, focusY{};
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
		using ringBufferType = RingBuffer<ControlAction, 256>;
		using const_iterator = ringBufferType::const_iterator;
		ringBufferType _actions;

	public:

		void clear();
		uiw size() const;

        template <typename T> void Enqueue(DeviceType device, T action)
        {
            ASSUME(device != DeviceType::_None);
            _actions.push_back(ControlAction{action, TimeMoment::Now(), device});
        }

		std::experimental::generator<ControlAction> Enumerate() const;
	};

	class KeyController : public std::enable_shared_from_this<KeyController>
	{
    public:
        using ListenerCallbackType = function<bool(const ControlAction &action)>; // must return true if the action needs to be blocked from going to any subsequent listeners
        using ListenerHandle = TListenerHandle<KeyController, ui32>;

        struct PositionInfo
        {
            i32 x{}, y{};
        };

        struct KeyInfo
        {
            using KeyStateType = ControlAction::Key::KeyStateType;
            KeyStateType keyState{};
            ui32 timesKeyStateChanged{};
            TimeMoment occuredAt = TimeMoment::Now();

            bool IsPressed() const;
        };
        
        using AllKeyStates = array<KeyInfo, (size_t)vkeyt::_size>;

    protected:
        KeyController();
        KeyController(KeyController &&) = delete;
        KeyController &operator = (KeyController &&) = delete;

	public:
        static shared_ptr<KeyController> New();

        ~KeyController() = default;
		void Dispatch(const ControlAction &action);
		void Dispatch(std::experimental::generator<ControlAction> enumerable);
		void Update(); // may be used for key repeating
		KeyInfo GetKeyInfo(vkeyt key, DeviceType device = DeviceType::MouseKeyboard) const; // always default for Touch
        optional<PositionInfo> GetPositionInfo(DeviceType device = DeviceType::MouseKeyboard) const; // always nullopt for Keyboard and Joystick
        const AllKeyStates &GetAllKeyStates(DeviceType device = DeviceType::MouseKeyboard) const; // always default for Touch
        ListenerHandle AddListener(const ListenerCallbackType &callback, DeviceType deviceMask);
		void RemoveListener(ListenerHandle &handle);

	private:
        NOINLINE ui32 FindIDForListener() const;

        struct MessageListener
        {
            ListenerCallbackType listener{};
            DeviceType deviceMask{};
            ui32 id{};
        };

        vector<MessageListener> _listeners{};
        bool _isListenersDirty = false;
        bool _isDispatchingInProgress = false;
        ui32 _currentId = 0;
        array<AllKeyStates, 1> _mouseKeyboardKeyStates{};
        array<AllKeyStates, 8> _joystickKeyStates{};
        const array<AllKeyStates, 1> _defaultKeyStates{};
        array<optional<PositionInfo>, 1> _mousePositionInfos{};
        array<optional<PositionInfo>, 10> _touchPositionInfos{};
	};
}