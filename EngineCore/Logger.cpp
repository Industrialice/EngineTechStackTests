#include "BasicHeader.hpp"
#include "Logger.hpp"

using namespace EngineCore;

void Logger::RemoveListener(LoggerLocation *instance, void *handle)
{
    instance->logger->RemoveListener(*(ListenerHandle *)handle);
}

Logger::Logger()
{
    _loggerLocation = make_shared<LoggerLocation>(*this);
}

Logger::Logger(Logger &&source) noexcept
{
    _listeners = move(source._listeners);
    _currentId = source._currentId;
    _isEnabled = source._isEnabled.load();
    _isThreadSafe = source._isThreadSafe.load();
    _loggerLocation = move(source._loggerLocation);
    _loggerLocation->logger = this;
}

Logger &Logger::operator = (Logger &&source) noexcept
{
    _listeners = move(source._listeners);
    _currentId = source._currentId;
    _isEnabled = source._isEnabled.load();
    _isThreadSafe = source._isThreadSafe.load();
    _loggerLocation = move(source._loggerLocation);
    _loggerLocation->logger = this;
    return *this;
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

    for (auto it = _listeners.rbegin(); it != _listeners.rend(); ++it)
    {
        const auto &listener = *it;
        if ((listener.levelMask + level) == listener.levelMask)
        {
            listener.callback(level, string_view(_logBuffer, (size_t)printed));
        }
    }
}

auto Logger::OnMessage(const ListenerCallbackType &listener, LogLevel levelMask) -> ListenerHandle
{
    optional<std::scoped_lock<std::mutex>> scopeLock;
    if (_isThreadSafe.load())
    {
        scopeLock.emplace(_mutex);
    }

    if (levelMask == LogLevel::_None) // not an error, but probably an unexpected case
    {
        SOFTBREAK;
        return {};
    }

    ui32 id = AssignId<MessageListener, ui32, &MessageListener::id>(_currentId, _listeners.begin(), _listeners.end());
    _listeners.push_back({listener, levelMask, id});
    return {_loggerLocation, id};
}

void Logger::RemoveListener(ListenerHandle &handle)
{
    optional<std::scoped_lock<std::mutex>> scopeLock;
    if (_isThreadSafe.load())
    {
        scopeLock.emplace(_mutex);
    }

    weak_ptr<ListenerHandle::ownerType> currentOwner{};
    handle.Owner().swap(currentOwner);
    if (currentOwner.expired())
    {
        return;
    }

    ASSUME(Funcs::AreSharedPointersEqual(handle.Owner(), _loggerLocation));

    for (auto it = _listeners.begin(); ; ++it)
    {
        ASSUME(it != _listeners.end());
        if (it->id == handle.Id())
        {
            _listeners.erase(it);
            break;
        }
    }
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
