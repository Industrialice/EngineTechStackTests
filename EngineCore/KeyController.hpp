#pragma once

#include "VirtualKeys.hpp"
#include "RingBuffer.h"

// TODO: window went active/inactive, multiple windows handling

namespace EngineCore
{
    // don't forget to change DeviceType_ConstantsCount in case you update DeviceType
	enum class DeviceType : ui32 { 
		MouseKeyboard0 = (1 << 0), MouseKeyboard1 = (1 << 1), MouseKeyboard2 = (1 << 2), MouseKeyboard3 = (1 << 3), 
		Joystick0 = (1 << 4), Joystick1 = (1 << 5), Joystick2 = (1 << 6), Joystick3 = (1 << 7), Joystick4 = (1 << 8), Joystick5 = (1 << 9), Joystick6 = (1 << 10), Joystick7 = (1 << 11) };
    constexpr ui32 DeviceType_ConstantsCount = 12;

	constexpr DeviceType DeviceType_All = DeviceType(UINT32_MAX);
	constexpr DeviceType DeviceType_None = DeviceType(0);
	inline DeviceType operator - (DeviceType left, DeviceType right) { return DeviceType((ui32)left & ~(ui32)right); }
	inline DeviceType operator + (DeviceType left, DeviceType right) { return DeviceType((ui32)left | (ui32)right); }

	struct ControlAction
	{
		struct Key
		{
            vkeyt key{};
			enum class KeyStateType { Pressed, Released, Repeated } keyState;
		};
		struct MouseMove
		{
            i32 deltaX{}, deltaY{};
		};
		struct MouseWheel
		{
            i32 delta{};
		};

        variant<Key, MouseMove, MouseWheel> action{};
        chrono::time_point<chrono::steady_clock> occuredAt{};
		DeviceType deviceType = DeviceType::MouseKeyboard0;

		ControlAction() {}
		ControlAction(vkeyt key, Key::KeyStateType state, const decltype(occuredAt) &occuredAt, DeviceType deviceType) : action{ Key{key, state} }, occuredAt{ occuredAt }, deviceType{ deviceType } {}
		ControlAction(i32 deltaX, i32 deltaY, const decltype(occuredAt) &occuredAt, DeviceType deviceType) : action{ MouseMove{deltaX, deltaY} }, occuredAt{ occuredAt }, deviceType{ deviceType } {}
		ControlAction(i32 delta, const decltype(occuredAt) &occuredAt, DeviceType deviceType) : action{ MouseWheel{delta} }, occuredAt{ occuredAt }, deviceType{ deviceType } {}
	};

	class ControlsQueue
	{
		using ringBufferType = RingBuffer<ControlAction, 256>;
		using const_iterator = ringBufferType::const_iterator<>;
		ringBufferType _actions;

	public:

		void clear();
		uiw size() const;

		void EnqueueKey(DeviceType deviceType, vkeyt key, ControlAction::Key::KeyStateType keyState);
		void EnqueueMouseMove(DeviceType deviceType, i32 deltaX, i32 deltaY);
		void EnqueueMouseWheel(DeviceType deviceType, i32 delta);

		std::experimental::generator<ControlAction> Enumerate() const;
	};

	class KeyController : public std::enable_shared_from_this<KeyController>
	{
        struct ListenerAndMask;

    public:
        using ListenerCallbackType = function<bool(const ControlAction &action)>; // returns true if the key was blocked from going to any subsequent listeners

        class ListenerHandle
        {
            friend KeyController;

            weak_ptr<KeyController> _owner;
            const ListenerAndMask *_listenerAndMask;

            void Remove();

        public:
            ~ListenerHandle();
            ListenerHandle() = default;
            ListenerHandle(ListenerHandle &&source) = default;
            ListenerHandle &operator = (ListenerHandle &&source);
            ListenerHandle(const shared_ptr<KeyController> &keyController, const ListenerAndMask *listenerAndMask) : _owner(keyController), _listenerAndMask(listenerAndMask) {}
            bool operator == (const ListenerHandle &other) const;
            bool operator != (const ListenerHandle &other) const;
        };

    private:
		struct KeyInfo
		{
			using KeyStateType = ControlAction::Key::KeyStateType;
			KeyStateType keyState;
            ui32 timesKeyStateChanged;
			chrono::time_point<chrono::steady_clock> occuredAt;

            bool IsPressed() const;
		};

        struct ListenerAndMask
        {
            const ListenerCallbackType listener;
            DeviceType deviceMask;
        };

    protected:
        KeyController();
        KeyController(KeyController &&) = delete;
        KeyController &operator = (KeyController &&) = delete;

	public:
        static shared_ptr<KeyController> New();

        ~KeyController();
		void Dispatch(const ControlAction &action);
		void Dispatch(std::experimental::generator<ControlAction> enumerable);
		void Update(); // may be used for key repeating
		KeyInfo GetKeyInfo(vkeyt key, DeviceType deviceType = DeviceType::MouseKeyboard0) const;
        ListenerHandle AddListener(const ListenerCallbackType &callback, DeviceType deviceMask = DeviceType_All); // will try to find a listener with the same owner in the current set of listeners, if successful, will check the pointers, if they also correspond, will remove the currently added callback
		void RemoveListener(ListenerHandle &handle);

	private:
        struct ImmutableListener
        {
            const ListenerCallbackType listener{};
            DeviceType deviceMask{};
            const ListenerAndMask *listenerAndMask{};
        };

		// the reason behind 2 sets of listeners is this: while dispatching control events, new listeners may be added or removed
		// that should be allowed and is defined as taking effect right after the dispatching of a ControlAction is finished
		// immutable listeners set is used only for enumeration and is synchronized with mutable listeners before dispatching a ControlAction
        forward_list<ListenerAndMask> _mutableListeners{};
        vector<ImmutableListener> _immutableListeners{};
        bool _isMutableListenersDirty = false;
        bool _isCurrentlyEnumeratingImmutableListeners = false;
        array<array<KeyInfo, (size_t)vkeyt::_size>, DeviceType_ConstantsCount> _keyStates{};
	};
}