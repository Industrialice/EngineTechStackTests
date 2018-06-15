#pragma once

namespace EngineCore
{
    template <typename OwnerType, typename IdType> class TListenerHandle
    {
        IdType _id{};
        weak_ptr<OwnerType> _owner{};

    public:
        ~TListenerHandle()
        {
            const auto &strongOwner = _owner.lock();
            if (strongOwner != nullptr)
            {
                // it's bad to have this method name hardcoded, but if you use a template parameter, you will have problems defining such method itself
                // make sure that RemoveListener is thread safe if you have listeners in different threads
                strongOwner->RemoveListener(*this);
            }
        }

        TListenerHandle() = default;

        TListenerHandle(const weak_ptr<OwnerType> &owner, IdType id) : _owner(owner), _id(id)
        {}

        TListenerHandle(weak_ptr<OwnerType> &&owner, IdType id) : _owner(move(owner)), _id(id)
        {}

        TListenerHandle(TListenerHandle &&source) : _owner(move(source._owner)), _id(source._id)
        {
            source._owner = nullptr;
        }

        TListenerHandle &operator = (TListenerHandle &&source)
        {
            _owner = move(source._owner);
            _id = source._id;
            return *this;
        }

        bool operator == (const TListenerHandle &other) const
        {
            return _id == other._id && Funcs::AreSharedPointersEqual(_owner, other._owner);
        }

        bool operator != (const TListenerHandle &other) const
        {
            return !this->operator == (other);
        }

        IdType Id() const
        {
            return _id;
        }

        const weak_ptr<OwnerType> &Owner() const
        {
            return _owner;
        }

        weak_ptr<OwnerType> &Owner()
        {
            return _owner;
        }
    };

    // there's also possible implementation which would patch relevant addresses
    // no reason to add it now, but it might be useful in the future because it
    // removes necessity to have special shared object
}