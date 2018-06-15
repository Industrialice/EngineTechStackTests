#pragma once

#include "ListenerHandle.hpp"

namespace EngineCore
{
    ENUM_COMBINABLE(LogLevel, ui32,
        _None = 0,
        Error = Funcs::BitPos(0),
        Info = Funcs::BitPos(1),
        Warning = Funcs::BitPos(2),
        Critical = Funcs::BitPos(3),
        Debug = Funcs::BitPos(4),
        ImportantInfo = Funcs::BitPos(5),
        Other = Funcs::BitPos(6),
        _All = ui32_max);

    class Logger
    {
        struct LoggerLocation
        {
            Logger *logger;

            using ListenerHandle = TListenerHandle<LoggerLocation, ui32>;

            LoggerLocation(Logger *logger) : logger(logger) {}
            void RemoveListener(ListenerHandle &handle);
        };

    public:
        // note that if you have listeners in different threads,
        // moving or destroying logger will be a race condition
        Logger();
        Logger(Logger &&source);
        Logger &operator = (Logger &&source);

        using ListenerCallbackType = function<void(LogLevel, string_view)>;
        using ListenerHandle = LoggerLocation::ListenerHandle;

        NOINLINE void Message(LogLevel level, const char *format, ...); // printf-family function is going to be used to convert the message into a string
        ListenerHandle OnMessage(const ListenerCallbackType &listener, LogLevel levelMask = LogLevel::_All);
        void RemoveListener(ListenerHandle &handle);
        void IsEnabled(bool isEnabled);
        bool IsEnabled() const;
        void IsThreadSafe(bool isSafe);
        bool IsThreadSafe() const;

    private:        
        struct MessageListener
        {
            ListenerCallbackType callback{};
            LogLevel levelMask{};
            ui32 id{};
        };

        static constexpr uiw logBufferSize = 65536;

        std::mutex _mutex{};
        vector<MessageListener> _listeners{};
        ui32 _currentId = 0;
        std::atomic<bool> _isEnabled{true};
        std::atomic<bool> _isThreadSafe{false};
        char _logBuffer[logBufferSize];
        shared_ptr<LoggerLocation> _loggerLocation{};
    };
}