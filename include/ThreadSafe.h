#pragma once

#include <optional>
#include <queue>
#include <atomic>
#include <set>
#include <Threading.h>
#include <deque>
#include <ranges>

template <typename T>
class TSQueue
{
	std::queue<T> _queue;
	std::atomic_flag locked = ATOMIC_FLAG_INIT;

public:
	void Push(T val)
	{
		while (locked.test_and_set(std::memory_order_acquire)) {
			;
		}
		_queue.push(val);
		locked.clear(std::memory_order_release);
	}

	void Pop()
	{
		while (locked.test_and_set(std::memory_order_acquire)) {
			;
		}
		std::optional<T> val = _queue.empty() ? std::optional<T>() : std::optional<T>(_queue.front());
		_queue.pop();
		locked.clear(std::memory_order_release);
	}

	bool Empty()
	{
		while (locked.test_and_set(std::memory_order_acquire)) {
			;
		}
		bool empty = _queue.empty();
		locked.clear(std::memory_order_release);
		return empty;
	}

	size_t Size()
	{
		while (locked.test_and_set(std::memory_order_acquire)) {
			;
		}
		size_t size = _queue.size();
		locked.clear(std::memory_order_release);
		return size;
	}
};

template <class Key, class Compare = std::less<Key>, class Allocator = std::allocator<Key>>
class ts_set
{
private:
	std::atomic_flag _flag = ATOMIC_FLAG_INIT;
	std::set<Key, Compare, Allocator> _set;

public:

	ts_set()
	{
	}

	template <class _Iter>
	ts_set(_Iter _First, _Iter _Last)
	{
		SpinlockA guard(_flag);
		_set->insert(_First, _Last);
	}

	template <class _Iter>
	ts_set(_Iter _First, _Iter _Last, const Compare& _Pred)
	{
		SpinlockA guard(_flag);
		_set->insert(_First, _Last, _Pred);
	}

	template <class _Iter>
	ts_set(_Iter _First, _Iter _Last, const Allocator& _Al)
	{
		SpinlockA guard(_flag);
		_set->insert(_First, _Last,_Al);
	}

	template <class _Iter>
	ts_set(_Iter _First, _Iter _Last, const Compare& _Pred, const Allocator& _Al)
	{
		SpinlockA guard(_flag);
		_set->insert(_First, _Last,_Pred, _Al);
	}

	ts_set(std::initializer_list<Key> _Ilist)
	{
		SpinlockA guard(_flag);
		_set->insert(_Ilist);
	}

	ts_set(std::initializer_list<Key> _Ilist, const Compare& _Pred)
	{
		SpinlockA guard(_flag);
		_set->insert(_Ilist, _Pred);
	}

	ts_set(std::initializer_list<Key> _Ilist, const Allocator& _Al)
	{
		SpinlockA guard(_flag);
		_set->insert(_Ilist,_Al);
	}

	ts_set(std::initializer_list<Key> _Ilist, const Compare& _Pred, const Allocator& _Al)
	{
		SpinlockA guard(_flag);
		_set->insert(_Ilist, _Pred, _Al);
	}

	ts_set& operator=(std::initializer_list<Key> _Ilist)
	{
		SpinlockA guard(_flag);
		_set->clear();
		_set->insert(_Ilist);
		return *this;
	}

	auto begin()
	{
		SpinlockA guard(_flag);
		return _set.begin();
	}

	auto end()
	{
		SpinlockA guard(_flag);
		return _set.end();
	}

	auto rbegin()
	{
		SpinlockA guard(_flag);
		return _set.rbegin();
	}

	auto rend()
	{
		SpinlockA guard(_flag);
		return _set.rend();
	}

	bool empty()
	{
		SpinlockA guard(_flag);
		return _set.empty();
	}

	size_t size()
	{
		SpinlockA guard(_flag);
		return _set.size();
	}

	size_t max_size()
	{
		SpinlockA guard(_flag);
		return _set.max_size();
	}

	void clear()
	{
		SpinlockA guard(_flag);
		_set.clear();
	}

	void insert(const Key& value)
	{
		SpinlockA guard(_flag);
		_set.insert(value);
	}

	void insert(Key&& value)
	{
		SpinlockA guard(_flag);
		_set.insert(value);
	}

	void insert(std::set<Key, Compare, Allocator>::const_iterator pos, const Key& value)
	{
		SpinlockA guard(_flag);
		_set.insert(pos, value);
	}
	void insert(std::set<Key, Compare, Allocator>::const_iterator pos, Key&& value)
	{
		SpinlockA guard(_flag);
		_set.insert(pos, value);
	}
	void insert(std::initializer_list<Key> ilist)
	{
		SpinlockA guard(_flag);
		_set.insert(ilist);
	}
};

template <class T, class Allocator = std::pmr::polymorphic_allocator<T>>
class ts_deque
{
private:
	std::atomic_flag _flag = ATOMIC_FLAG_INIT;
	std::deque<T, Allocator> _queue;

	void lock()
	{
		while (_flag.test_and_set(std::memory_order_acquire));
	}
	void unlock()
	{
		_flag.clear(std::memory_order_release);
	}

public:
	std::deque<T, Allocator>& data()
	{
		return _queue;
	}
	void swap(ts_deque& other) noexcept
	{
		Spinlock guard(_flag);
		other.lock();
		other.data().swap(_queue);
		other.unlock();
	}

	explicit ts_deque(const Allocator& alloc = Allocator())
	{
		Spinlock guard(_flag);
		_queue = std::deque<T, Allocator>(alloc);
	}
	explicit ts_deque(size_t count, const Allocator& alloc = Allocator())
	{
		Spinlock guard(_flag);
		_queue = std::deque<T, Allocator>(count, alloc);
	}
	explicit ts_deque(size_t count, const T& value = T(), const Allocator& alloc = Allocator())
	{
		Spinlock guard(_flag);
		_queue = std::deque<T, Allocator>(count, value, alloc);
	}
	template <class InputIt>
	ts_deque(InputIt first, InputIt last, const Allocator& alloc = Allocator())
	{
		Spinlock guard(_flag);
		_queue = std::deque<T, Allocator>(first, last, alloc);
	}
	ts_deque(const std::deque<T, Allocator>& other)
	{
		Spinlock guard(_flag);
		_queue = std::deque<T, Allocator>(other);
	}
	ts_deque(std::deque<T, Allocator>&& other)
	{
		Spinlock guard(_flag);
		_queue = std::deque<T, Allocator>(other);
	}
	ts_deque(const std::deque<T, Allocator>& other, const std::type_identity_t<Allocator>& alloc)
	{
		Spinlock guard(_flag);
		_queue = std::deque<T, Allocator>(other, alloc);
	}
	ts_deque(std::deque<T, Allocator>&& other, const std::type_identity_t<Allocator>& alloc)
	{
		Spinlock guard(_flag);
		_queue = std::deque<T, Allocator>(other, alloc);
	}
	ts_deque(std::initializer_list<T> init, const Allocator& alloc = Allocator())
	{
		Spinlock guard(_flag);
		_queue = std::deque<T, Allocator>(init, alloc);
	}

	ts_deque(const ts_deque& other)
	{
		Spinlock guard(_flag);
		other.lock();
		_queue = std::deque<T, Allocator>(other.data());
		other.unlock();
	}
	ts_deque(ts_deque&& other)
	{
		Spinlock guard(_flag);
		other.lock();
		_queue = std::deque<T, Allocator>(other.data());
		other.unlock();
	}
	ts_deque(const ts_deque& other, const std::type_identity_t<Allocator>& alloc)
	{
		Spinlock guard(_flag);
		other.lock();
		_queue = std::deque<T, Allocator>(other.data(), alloc);
		other.unlock();
	}
	ts_deque(ts_deque&& other, const std::type_identity_t<Allocator>& alloc)
	{
		Spinlock guard(_flag);
		other.lock();
		_queue = std::deque<T, Allocator>(other.data(), alloc);
		other.unlock();
	}

	// Copy missing

	void assign(size_t count, const T& value)
	{
		Spinlock guard(_flag);
		_queue.assign(count, value);
	}
	template <class InputIt>
	void assign(InputIt first, InputIt last)
	{
		Spinlock guard(_flag);
		_queue.assign(first, last);
	}
	void assign(std::initializer_list<T> ilist)
	{
		Spinlock guard(_flag);
		_queue.assign(ilist);
	}
	std::deque<T, Allocator>::allocator_type get_allocator()
	{
		Spinlock guard(_flag);
		return _queue.get_allocator();
	}

	std::deque<T, Allocator>::value_type& at(std::deque<T, Allocator>::size_type pos)
	{
		Spinlock guard(_flag);
		return _queue.at(pos);
	}

	std::deque<T, Allocator>::value_type& operator[](std::deque<T, Allocator>::size_type pos)
	{
		Spinlock guard(_flag);
		return _queue[pos];
	}

	std::deque<T, Allocator>::value_type& front()
	{
		Spinlock guard(_flag);
		return _queue.front();
	}

	using type = std::deque<T, Allocator>::value_type;

	std::deque<T, Allocator>::value_type get_pop_front()
	{
		Spinlock guard(_flag);
		if (_queue.empty())
			throw std::out_of_range("out of range");
		type elem = _queue.front();
		_queue.pop_front();
		return elem;
	}

	std::deque<T, Allocator>::value_type& back()
	{
		Spinlock guard(_flag);
		return _queue.back();
	}

	std::deque<T, Allocator>::iterator begin()
	{
		Spinlock guard(_flag);
		return _queue.begin();
	}

	std::deque<T, Allocator>::const_iterator cbegin()
	{
		Spinlock guard(_flag);
		return _queue.cbegin();
	}

	std::deque<T, Allocator>::iterator end()
	{
		Spinlock guard(_flag);
		return _queue.end();
	}

	std::deque<T, Allocator>::const_iterator cend()
	{
		Spinlock guard(_flag);
		return _queue.cend();
	}

	std::deque<T, Allocator>::reverse_iterator rbegin()
	{
		Spinlock guard(_flag);
		return _queue.rbegin();
	}

	std::deque<T, Allocator>::const_reverse_iterator crbegin()
	{
		Spinlock guard(_flag);
		return _queue.crbegin();
	}

	std::deque<T, Allocator>::reverse_iterator rend()
	{
		Spinlock guard(_flag);
		return _queue.rend();
	}

	std::deque<T, Allocator>::const_reverse_iterator crend()
	{
		Spinlock guard(_flag);
		return _queue.crend();
	}

	bool empty()
	{
		Spinlock guard(_flag);
		return _queue.empty();
	}

	std::deque<T, Allocator>::size_type size()
	{
		Spinlock guard(_flag);
		return _queue.size();
	}

	std::deque<T, Allocator>::size_type max_size()
	{
		Spinlock guard(_flag);
		return _queue.max_size();
	}

	void shrink_to_fit()
	{
		Spinlock guard(_flag);
		_queue.shrink_to_fit();
	}

	void clear()
	{
		Spinlock guard(_flag);
		_queue.clear();
	}
	template <class... Args>
	std::deque<T, Allocator>::iterator emplace(std::deque<T, Allocator>::const_iterator pos, Args&&... args)
	{
		Spinlock guard(_flag);
		_queue.emplace(pos, std::forward<Args>(args)...);
	}
	template <class... Args>
	std::deque<T, Allocator>::value_type& emplace_back(Args&&... args)
	{
		Spinlock guard(_flag);
		_queue.emplace_back(std::forward<Args>(args)...);
	}
	template <class... Args>
	std::deque<T, Allocator>::value_type& emplace_front(Args&&... args)
	{
		Spinlock guard(_flag);
		_queue.emplace_front(std::forward<Args>(args)...);
	}

	void push_back(const T& value)
	{
		Spinlock guard(_flag);
		_queue.push_back(value);
	}
	void push_back(T&& value)
	{
		Spinlock guard(_flag);
		_queue.push_back(value);
	}
	void pop_back()
	{
		Spinlock guard(_flag);
		_queue.pop_back();
	}
	void push_front(const T& value)
	{
		Spinlock guard(_flag);
		_queue.push_front(value);
	}
	void push_front(T&& value)
	{
		Spinlock guard(_flag);
		_queue.push_front(value);
	}
	void pop_front()
	{
		Spinlock guard(_flag);
		_queue.pop_front();
	}
	void resize(std::deque<T, Allocator>::size_type count)
	{
		Spinlock guard(_flag);
		_queue.resize(count);
	}
	void resize(std::deque<T, Allocator>::size_type count, const std::deque<T, Allocator>::value_type& value)
	{
		Spinlock guard(_flag);
		_queue.resize(count, value);
	}

	std::deque<T, Allocator>::iterator erase(std::deque<T, Allocator>::const_iterator pos)
	{
		Spinlock guard(_flag);
		return _queue.erase(pos);
	}
	std::deque<T, Allocator>::iterator erase(std::deque<T, Allocator>::const_iterator first, std::deque<T, Allocator>::const_iterator last)
	{
		Spinlock guard(_flag);
		return _queue.erase(first, last);
	}
	std::deque<T, Allocator>::iterator insert(std::deque<T, Allocator>::const_iterator pos, const T& value)
	{
		Spinlock guard(_flag);
		return _queue.insert(pos, value);
	}
	std::deque<T, Allocator>::iterator insert(std::deque<T, Allocator>::const_iterator pos, T&& value)
	{
		Spinlock guard(_flag);
		return _queue.insert(pos, value);
	}
	std::deque<T, Allocator>::iterator insert(std::deque<T, Allocator>::const_iterator pos, std::deque<T, Allocator>::size_type count, const T& value)
	{
		Spinlock guard(_flag);
		return _queue.insert(pos, count, value);
	}
	template <class InputIt>
	std::deque<T, Allocator>::iterator insert(std::deque<T, Allocator>::const_iterator pos, InputIt first, InputIt last)
	{
		Spinlock guard(_flag);
		return _queue.insert(pos, first, last);
	}
	std::deque<T, Allocator>::iterator insert(std::deque<T, Allocator>::const_iterator pos, std::initializer_list<T> ilist)
	{
		Spinlock guard(_flag);
		return _queue.insert(pos, ilist);
	}

	std::deque<T, Allocator>::value_type& ts_front()
	{
		Spinlock guard(_flag);
		if (_queue.empty()) {
			throw std::out_of_range("empty");
		} else {
			auto& front = _queue.front();
			_queue.pop_front();
			return front;
		}
	}

	std::deque<T, Allocator>::value_type& ts_back()
	{
		Spinlock guard(_flag);
		if (_queue.empty()) {
			throw std::out_of_range("empty");
		} else {
			auto& back = _queue.back();
			_queue.pop_back();
			return back;
		}
	}
};