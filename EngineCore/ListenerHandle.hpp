#pragma once

namespace EngineCore
{
    template <typename OwnerType, typename IdType, typename StorageType = OwnerType> class TListenerHandle
    {
        friend OwnerType;

        IdType _id{};
        weak_ptr<StorageType> _owner{};

    public:
        ~TListenerHandle()
        {
            const auto &strongOwner = _owner.lock();
            if (strongOwner != nullptr)
            {
                strongOwner->RemoveListener(*this); // it's bad to have this method name hardcoded, but if you use a template parameter, you will have problems defining such method itself
            }
        }

        TListenerHandle() = default;

        TListenerHandle(const weak_ptr<StorageType> &owner, IdType id) : _owner(owner), _id(id)
        {}

        TListenerHandle(weak_ptr<StorageType> &&owner, IdType id) : _owner(move(owner)), _id(id)
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
    };

    // there's also possible implementation which would patch relevant addresses
    // no reason to add it now, but it might be useful in the future because it
    // removes necessity to have special shared object
}