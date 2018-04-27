#pragma once

namespace EngineCore
{
	enum class LogLevel : ui32 { Error = (1 << 0), Info = (1 << 1), Warning = (1 << 2), Critical = (1 << 3), Debug = (1 << 4), ImportantInfo = (1 << 5), Other = (1 << 6) };

	constexpr LogLevel LogLevel_All = LogLevel(UINT32_MAX);
	constexpr LogLevel LogLevel_None = LogLevel(0);
	inline LogLevel operator - (LogLevel left, LogLevel right) { return LogLevel((uiw)left & ~(uiw)right); }
	inline LogLevel operator + (LogLevel left, LogLevel right) { return LogLevel((uiw)left | (uiw)right); }

	class Logger : public std::enable_shared_from_this<Logger>
	{
        struct MessageListener;

    protected:
        Logger(Logger &&) = delete;
        Logger &operator = (Logger &&) = delete;
        Logger();

	public:
        static shared_ptr<Logger> New();

		using ListenerCallbackType = function<void(LogLevel, string_view)>;

        struct ListenerHandle
        {
            weak_ptr<Logger> _owner;
            const MessageListener *_messageListener;

            void Remove();

        public:
            ~ListenerHandle();
            ListenerHandle() = default;
            ListenerHandle(ListenerHandle &&source) = default;
            ListenerHandle &operator = (ListenerHandle &&source);
            ListenerHandle(const shared_ptr<Logger> &logger, const MessageListener *messageListener) : _owner(logger), _messageListener(messageListener) {}
            bool operator == (const ListenerHandle &other) const;
            bool operator != (const ListenerHandle &other) const;
        };

		void Message(LogLevel level, const char *format, ...); // printf-family function is going to be used to convert the message into a string
        ListenerHandle AddListener(const ListenerCallbackType &listener, LogLevel levelMask = LogLevel_All);
		void RemoveListener(ListenerHandle &handle);
        bool IsEnabled() const;
        void IsEnabled(bool isEnabled);

	private:
        struct MessageListener
        {
            const ListenerCallbackType callback;
            LogLevel levelMask;
        };

        forward_list<MessageListener> _listeners{};
        unique_ptr<char[]> _logBuffer{};
        mutex _lock{};
        atomic<bool> _isEnabled{true};

		static constexpr uiw logBufferSize = 65536;
	};
}