#pragma once

#include "ListenerHandle.hpp"

namespace EngineCore
{
    enum class LogLevel : ui32
    {
        Error = Funcs::BitPos(0),
        Info = Funcs::BitPos(1),
        Warning = Funcs::BitPos(2),
        Critical = Funcs::BitPos(3),
        Debug = Funcs::BitPos(4),
        ImportantInfo = Funcs::BitPos(5),
        Other = Funcs::BitPos(6)
    };

    constexpr LogLevel LogLevel_All = LogLevel(ui32_max);
    constexpr LogLevel LogLevel_None = LogLevel(0);
    inline LogLevel operator - (LogLevel left, LogLevel right) { return LogLevel((uiw)left & ~(uiw)right); }
    inline LogLevel operator + (LogLevel left, LogLevel right) { return LogLevel((uiw)left | (uiw)right); }
    inline LogLevel &operator -= (LogLevel &left, LogLevel right) { left = left - right; return left; }
    inline LogLevel &operator += (LogLevel &left, LogLevel right) { left = left + right; return left; }

    class Logger
    {
        struct LoggerLocation
        {
            Logger *logger;

            using ListenerHandle = TListenerHandle<Logger, ui32, LoggerLocation>;

            LoggerLocation(Logger *logger) : logger(logger) {}
            void RemoveListener(ListenerHandle &handle);
        };

    public:
        Logger();
        Logger(const Logger &source);
        Logger &operator = (const Logger &source);
        Logger(Logger &&source);
        Logger &operator = (Logger &&source);

        using ListenerCallbackType = function<void(LogLevel, string_view)>;
        using ListenerHandle = LoggerLocation::ListenerHandle;

        NOINLINE void Message(LogLevel level, const char *format, ...); // printf-family function is going to be used to convert the message into a string
        ListenerHandle AddListener(const ListenerCallbackType &listener, LogLevel levelMask = LogLevel_All);
        void RemoveListener(ListenerHandle &handle);
        void IsEnabled(bool isEnabled);
        bool IsEnabled() const;
        void IsThreadSafe(bool isSafe);
        bool IsThreadSafe() const;

    private:
        NOINLINE ui32 FindIDForListener() const;
        
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