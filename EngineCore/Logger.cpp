#include "BasicHeader.hpp"
#include "Logger.hpp"

#ifndef EXTASY_DISABLE_LOGGING

using namespace EngineCore;

Logger::Logger()
{}

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

    optional<std::scoped_lock<std::mutex>> scopeLock;
    if (_isThreadSafe.load())
    {
        scopeLock.emplace(_mutex);
    }

    if (_listeners.empty())
    {
        return;
    }

    va_list args;
    va_start(args, format);

    int printed = vsnprintf(_logBuffer, logBufferSize, format, args);

    va_end(args);

    if (printed <= 0)
    {
        return;
    }

    for (const auto &listener : _listeners)
    {
        if ((listener.levelMask + level) == listener.levelMask)
        {
            listener.callback(level, string_view(_logBuffer, (size_t)printed));
        }
    }
}

auto Logger::AddListener(const ListenerCallbackType &listener, LogLevel levelMask) -> ListenerHandle
{
    optional<std::scoped_lock<std::mutex>> scopeLock;
    if (_isThreadSafe.load())
    {
        scopeLock.emplace(_mutex);
    }

    if ((ui32)levelMask == 0) // not an error, but probably an unexpected case
    {
        SOFTBREAK;
        return ListenerHandle();
    }

    _listeners.push_front({listener, levelMask});

    return ListenerHandle{shared_from_this(), &_listeners.front()};
}

void Logger::RemoveListener(ListenerHandleData &handle)
{
    optional<std::scoped_lock<std::mutex>> scopeLock;
    if (_isThreadSafe.load())
    {
        scopeLock.emplace(_mutex);
    }

    const auto &strongOwner = handle._owner.lock();
    if (strongOwner == nullptr)
    {
        return;
    }
    if (!Funcs::AreSharedPointersEqual(strongOwner, shared_from_this()))
    {
        SOFTBREAK;
        return;
    }

    _listeners.remove_if([&handle](const MessageListener &value) { return &value == handle._messageListener; });

    handle._owner.reset();
}

void Logger::IsEnabled(bool isEnabled)
{
    _isEnabled.store(isEnabled);
}

bool Logger::IsEnabled() const
{
    return _isEnabled.load();
}
void Logger::IsThreadSafe(bool isSafe)
{
    _isThreadSafe.store(isSafe);
}

bool Logger::IsThreadSafe() const
{
    return _isThreadSafe.load();
}

void Logger::ListenerHandleData::Remove()
{
    const auto &strongOwner = _owner.lock();
    if (strongOwner != nullptr)
    {
        strongOwner->RemoveListener(*this);
    }
}

bool Logger::ListenerHandleData::operator == (const ListenerHandleData &other) const
{
    return Funcs::AreSharedPointersEqual(_owner, other._owner) && _messageListener == other._messageListener;
}

#endif