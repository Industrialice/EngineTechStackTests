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

    class Logger : public std::enable_shared_from_this<Logger>
    {
        struct MessageListener;

        struct ListenerHandleData
        {
            weak_ptr<Logger> _owner{};
            const MessageListener *_messageListener{};

            void Remove();
            bool operator == (const ListenerHandleData &other) const;
        };

    protected:
        Logger(Logger &&) = delete;
        Logger &operator = (Logger &&) = delete;
        Logger();

    public:
        static shared_ptr<Logger> New();

        using ListenerCallbackType = function<void(LogLevel, string_view)>;

        using ListenerHandle = TListenerHandle<ListenerHandleData>;

        void Message(LogLevel level, const char *format, ...); // printf-family function is going to be used to convert the message into a string
        ListenerHandle AddListener(const ListenerCallbackType &listener, LogLevel levelMask = LogLevel_All);
        void RemoveListener(ListenerHandleData &handle);
        void IsEnabled(bool isEnabled);
        bool IsEnabled() const;
        // these methods aren't thread safe themselves, call them from the main thread only
        void IsThreadSafe(bool isSafe);
        bool IsThreadSafe() const;

    private:
        struct MessageListener
        {
            const ListenerCallbackType callback;
            LogLevel levelMask;
        };

        static constexpr uiw logBufferSize = 65536;

        std::mutex _mutex{};
        forward_list<MessageListener> _listeners{};
        std::atomic<bool> _isEnabled{true};
        std::atomic<bool> _isThreadSafe{false};
        char _logBuffer[logBufferSize];
    };
}