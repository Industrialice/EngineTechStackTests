#pragma once

#include "ListenerHandle.hpp"

namespace EngineCore
{
    struct LogLevels
    {
        static constexpr struct LogLevel : EnumCombinable<LogLevel, ui32, true>
        {} _None = LogLevel::Create(0),
            Info = LogLevel::Create(1 << 0),
            Error = LogLevel::Create(1 << 1),
            Warning = LogLevel::Create(1 << 2),
            Critical = LogLevel::Create(1 << 3),
            Debug = LogLevel::Create(1 << 4),
            Attention = LogLevel::Create(1 << 5),
            Other = LogLevel::Create(1 << 6),
            _All = LogLevel::Create(ui32_max);
    };

    class Logger
    {
        struct LoggerLocation;
        static void RemoveListener(LoggerLocation *instance, void *handle);

        struct LoggerLocation
        {
            Logger *logger;

            using ListenerHandle = TListenerHandle<LoggerLocation, ui32, RemoveListener>;

            LoggerLocation(Logger &logger) : logger(&logger) {}
        };

    public:
        using ListenerCallbackType = function<void(LogLevels::LogLevel, string_view)>;
        using ListenerHandle = LoggerLocation::ListenerHandle;

        // note that if you have listeners in different threads,
        // moving or destroying the logger will be a race condition
        Logger();
        Logger(Logger &&source) noexcept;
        Logger &operator = (Logger &&source) noexcept;

        NOINLINE void Message(LogLevels::LogLevel level, const char *format, ...); // printf-family function will be used to convert the message into a string
        ListenerHandle OnMessage(const ListenerCallbackType &listener, LogLevels::LogLevel levelMask = LogLevels::_All);
        void RemoveListener(ListenerHandle &handle);
        void IsEnabled(bool isEnabled);
        bool IsEnabled() const;
        void IsThreadSafe(bool isSafe);
        bool IsThreadSafe() const;

    private:        
        struct MessageListener
        {
            ListenerCallbackType callback{};
            LogLevels::LogLevel levelMask{};
            ui32 id{};
        };

        static constexpr uiw logBufferSize = 65536;

        vector<MessageListener> _listeners{};
        std::atomic<bool> _isEnabled{true};
        std::atomic<bool> _isThreadSafe{false};
        ui32 _currentId = 0;
        shared_ptr<LoggerLocation> _loggerLocation{};
        std::mutex _mutex{};
        char _logBuffer[logBufferSize];
    };
}