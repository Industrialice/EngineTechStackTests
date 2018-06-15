#pragma once

#include "IKeyController.hpp"

namespace EngineCore
{
    class RecordingKeyController : public std::enable_shared_from_this<RecordingKeyController>, public IKeyController
    {
    protected:
        RecordingKeyController(const shared_ptr<IKeyController> &nextController);
        RecordingKeyController(RecordingKeyController &&) = delete;
        RecordingKeyController &operator = (RecordingKeyController &&) = delete;

    public:
        static shared_ptr<RecordingKeyController> New(const shared_ptr<IKeyController> &nextController);

        virtual ~RecordingKeyController() = default;
        virtual void Dispatch(const ControlAction &action) override;
        virtual void Dispatch(std::experimental::generator<ControlAction> enumerable) override;
        virtual void Update() override;
        [[nodiscard]] virtual KeyInfo GetKeyInfo(vkeyt key, DeviceType device = DeviceType::MouseKeyboard) const override;
        [[nodiscard]] virtual optional<i32Vector2> GetPositionInfo(DeviceType device = DeviceType::MouseKeyboard) const override;
        [[nodiscard]] virtual const AllKeyStates &GetAllKeyStates(DeviceType device = DeviceType::MouseKeyboard) const override;
        [[nodiscard]] virtual ListenerHandle OnControlAction(const ListenerCallbackType &callback, DeviceType deviceMask) override;
        virtual void RemoveListener(ListenerHandle &handle) override;
        [[nodiscard]] ListenerHandle OnRecordingControlAction(const ListenerCallbackType &callback); // you can use RemoveListener to detach

    private:        
        struct MessageListener
        {
            ListenerCallbackType listener{};
            ui32 id{};
        };
        shared_ptr<IKeyController> _nextController{};
        ui32 _currentId = 0;
        vector<MessageListener> _recordingListenerHandles{};
    };
}