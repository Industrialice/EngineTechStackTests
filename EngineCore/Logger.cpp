#include "BasicHeader.hpp"
#include "Logger.hpp"

using namespace EngineCore;

Logger::Logger()
{
    assert(_isEnabled.is_lock_free());
	_logBuffer = make_unique<char[]>(logBufferSize);
}

shared_ptr<Logger> Logger::New()
{
    struct Proxy : public Logger
    {
        Proxy() : Logger() {}
    };
    return make_shared<Proxy>();
}

void Logger::Message(LogLevel level, const char *format, ...)
{
    if (_isEnabled.load() == false)
    {
        return;
    }

    std::lock_guard<mutex> guard(_lock);

	if (_listeners.empty())
	{
		return;
	}

	va_list args;
	va_start(args, format);

	int printed = vsnprintf(_logBuffer.get(), logBufferSize, format, args);

	va_end(args);

	if (printed <= 0)
	{
		return;
	}

	for (const auto &listener : _listeners)
	{
        if ((listener.levelMask + level) == listener.levelMask)
        {
            listener.callback(level, string_view(_logBuffer.get(), (size_t)printed));
        }
	}
}

auto Logger::AddListener(const ListenerCallbackType &listener, LogLevel levelMask) -> ListenerHandle
{
    std::lock_guard<mutex> guard(_lock);

    if ((ui32)levelMask == 0) // not an error, but probably an unexpected case
    {
        SOFTBREAK;
        return ListenerHandle();
    }

    _listeners.push_front({listener, levelMask});

    return ListenerHandle{shared_from_this(), &_listeners.front()};
}

void Logger::RemoveListener(ListenerHandle &handle)
{
    std::lock_guard<mutex> guard(_lock);

    const auto &strongOwner = handle._owner.lock();
    if (strongOwner == nullptr)
    {
        return;
    }
    if (strongOwner.owner_before(shared_from_this()) == true || shared_from_this().owner_before(strongOwner) == true)
    {
        SOFTBREAK;
        return;
    }

    _listeners.remove_if([&handle](const MessageListener &value) { return &value == handle._messageListener; });

    handle._owner.reset();
}

bool Logger::IsEnabled() const
{
    return _isEnabled.load();
}

void Logger::IsEnabled(bool isEnabled)
{
    _isEnabled.store(isEnabled);
}

void Logger::ListenerHandle::Remove()
{
    const auto &strongOwner = _owner.lock();
    if (strongOwner != nullptr)
    {
        strongOwner->RemoveListener(*this);
    }
}

Logger::ListenerHandle::~ListenerHandle()
{
    Remove();
}

auto Logger::ListenerHandle::operator=(ListenerHandle &&source) -> ListenerHandle &
{
    assert(this != &source);
    Remove();
    this->_owner = move(source._owner);
    this->_messageListener = move(source._messageListener);
    return *this;
}

bool Logger::ListenerHandle::operator == (const ListenerHandle &other) const
{
    return (_owner.owner_before(other._owner) == false && other._owner.owner_before(_owner) == false) && _messageListener == other._messageListener;
}

bool Logger::ListenerHandle::operator != (const ListenerHandle &other) const
{
    return !(this->operator==(other));
}