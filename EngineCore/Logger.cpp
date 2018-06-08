#include "BasicHeader.hpp"
#include "Logger.hpp"

using namespace EngineCore;

Logger::Logger()
{
    _loggerLocation = make_shared<LoggerLocation>(this);
}

Logger::Logger(const Logger &source)
{
    _isEnabled = source._isEnabled.load();
    _isThreadSafe = source._isThreadSafe.load();
    _loggerLocation = make_shared<LoggerLocation>(this);
}

Logger &Logger::operator = (const Logger &source)
{
    if (this != &source)
    {
        _isEnabled = source._isEnabled.load();
        _isThreadSafe = source._isThreadSafe.load();
        _loggerLocation = make_shared<LoggerLocation>(this);
    }
    return *this;
}

Logger::Logger(Logger &&source)
{
    _listeners = move(source._listeners);
    source._listeners.clear();
    _currentId = source._currentId;
    _isEnabled = source._isEnabled.load();
    _isThreadSafe = source._isThreadSafe.load();
    _loggerLocation = move(source._loggerLocation);
    _loggerLocation->logger = this;
}

Logger &Logger::operator = (Logger &&source)
{
    _listeners = move(source._listeners);
    source._listeners.clear();
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
        return {};
    }

    ui32 id;
    if (_currentId != ui32_max)
    {
        id = _currentId;
        ++_currentId;
    }
    else
    {
        id = FindIDForListener();
    }

    _listeners.push_back({listener, levelMask, id});

    return {_loggerLocation, id};
}

void Logger::RemoveListener(ListenerHandle &handle)
{
    if (handle._owner.expired())
    {
        return;
    }

    ASSUME(Funcs::AreSharedPointersEqual(handle._owner, _loggerLocation));

    optional<std::scoped_lock<std::mutex>> scopeLock;
    if (_isThreadSafe.load())
    {
        scopeLock.emplace(_mutex);
    }

    for (auto it = _listeners.begin(); ; ++it)
    {
        ASSUME(it != _listeners.end());
        if (it->id == handle._id)
        {
            _listeners.erase(it);
            break;
        }
    }

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

NOINLINE ui32 Logger::FindIDForListener() const
{
    // this should never happen unless you have bogus code that calls AddListener/RemoveListener repeatedly
    // you'd need 50k AddListener calls per second to exhaust ui32 within 24 hours
    SOFTBREAK;
    return FindSmallestID<MessageListener, ui32, &MessageListener::id>(_listeners.begin(), _listeners.end());
}

void Logger::LoggerLocation::RemoveListener(ListenerHandle &handle)
{
    logger->RemoveListener(handle);
}
