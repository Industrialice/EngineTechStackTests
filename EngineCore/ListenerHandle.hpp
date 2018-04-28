#pragma once

namespace EngineCore
{
    template <typename T> struct TListenerHandle : public T
    {
        ~TListenerHandle()
        {
            Remove();
        }

        TListenerHandle() = default;

        TListenerHandle(TListenerHandle &&source) : T(move(source))
        {
            (T &)source = T();
        }

        TListenerHandle &operator = (TListenerHandle &&source)
        {
            T::operator = (move(source));
            (T &)source = T();
            return *this;
        }

        template <typename... Args> TListenerHandle(Args &&... args) : T{std::forward<Args>(args)...}
        {}

        bool operator == (const TListenerHandle &other) const
        {
            return T::operator == (other);
        }

        bool operator != (const TListenerHandle &other) const
        {
            return !this->operator == (other);
        }

        void Remove()
        {
            T::Remove();
        }
    };
}