#pragma once

// a simple implementation only for POD types
template <typename T, uiw MaxElements> class RingBuffer
{
    // TODO: is is_trivially_copyable_v correct?
    static_assert(std::is_trivially_copyable_v<T>, "only trivial types are allowed for this implementation");

	static constexpr bool isOverflowWrapped = MaxElements == 256 || MaxElements == 65536;
	using indexType = 
        conditional_t<MaxElements == 65536, ui16, 
        conditional_t<MaxElements == 256, ui8,
        conditional_t<MaxElements <= ui32_max, ui32, uiw>>>;

	array<T, MaxElements> _elements;
	indexType _head = 0; // index of the most recently added element
	indexType _tail = 0; // index of the oldest element
	// if _head == _tail when _isEmtpy == 0, then there's only one element, in all other cases, _head won't be equal _tail
	// the buffer grows into positive values
	indexType _isEmtpy = 1;

	/*
	----------------------------
	|free|  elements  |  free  |
	   tail         head
	*/

	static indexType AdvanceForward(indexType value)
	{
		if (false == isOverflowWrapped && value == MaxElements - 1)
		{
			value = 0;
		}
		else
		{
			++value;
		}
		return value;
	}

	static indexType AdvanceBackward(indexType value)
	{
		if (false == isOverflowWrapped && value == 0)
		{
			value = MaxElements - 1;
		}
		else
		{
			--value;
		}
		return value;
	}

public:
	static constexpr uiw MaxElements = MaxElements;

    RingBuffer() = default;

	void clear()
	{
		_isEmtpy = 1;
		_head = _tail;
	}

	uiw size() const
	{
		if (_isEmtpy)
		{
			return 0;
		}
		if (_head >= _tail)
		{
			return (uiw)_head - (uiw)_tail + 1;
		}
		return (uiw)_head + MaxElements - (uiw)_tail + 1;
	}

	void push_back(const T &value)
	{
		if (!_isEmtpy)
		{
			_head = AdvanceForward(_head);
		}
		_elements[_head] = value;
		if (_head == _tail && !_isEmtpy)
		{
			_tail = AdvanceForward(_tail);
		}
		_isEmtpy = 0;
	}

	void pop_back()
	{
		assert(_isEmtpy == 0);
		if (_head == _tail)
		{
			_isEmtpy = 1;
		}
		else
		{
			_head = AdvanceBackward(_head);
		}
	}

	template <typename T = T, uiw MaxElements = MaxElements, typename IndexType = indexType> class _const_iterator : public std::iterator<std::forward_iterator_tag, T>
	{
		static constexpr bool isOverflowWrapped = MaxElements == 256 || MaxElements == 65536;

		const T *_elements;
		IndexType _head, _tail;
		IndexType _isEmpty;

		static IndexType AdvanceForward(IndexType value)
		{
			if (false == isOverflowWrapped && value == MaxElements - 1)
			{
				value = 0;
			}
			else
			{
				++value;
			}
			return value;
		}

	public:
		_const_iterator(const T *elements, IndexType head, IndexType tail, IndexType isEmpty) : _elements(elements), _head(head), _tail(tail), _isEmpty(isEmpty)
		{}

		_const_iterator &operator ++ ()
		{
			if (_head == _tail)
			{
				assert(!_isEmpty);
				_isEmpty = 1;
			}
			else
			{
				_head = AdvanceForward(_head);
			}
			return *this;
		}

        _const_iterator &operator ++ (int)
		{
			auto ret = *this;
			this->operator++();
			return ret;
		}

		bool operator == (const _const_iterator &other) const
		{
			return _head == other._head && _isEmpty == other._isEmpty;
		}

		bool operator != (const _const_iterator &other) const
		{
			return !this->operator == (other);
		}

		const T &operator *() const
		{
			return _elements[_head];
		}

		const T *operator ->() const
		{
			return _elements[_head];
		}
	};

    using const_iterator = _const_iterator<>;

	auto begin() const
	{
		return cbegin();
	}

	auto cbegin() const
	{
		return const_iterator(_elements.data(), _tail, _head, _isEmtpy);
	}

	auto end() const
	{
		return cend();
	}

	auto cend() const
	{
		return const_iterator(_elements.data(), _head, _head, 1);
	}
};