#pragma once

#include <deque>
#include <vector>
#include <exception>
#include <stdexcept>

#include "Threading.h"

template <typename T>
class ThreadSafeVar
{
	T value;
	std::atomic_flag locked = ATOMIC_FLAG_INIT;

public:
	T Get()
	{
		while (locked.test_and_set(std::memory_order_acquire)) {
			;
		}
		T val = value;
		locked.clear(std::memory_order_release);
		return val;
	}

	void Set(T val)
	{
		while (locked.test_and_set(std::memory_order_acquire)) {
			;
		}
		value = val;
		locked.clear(std::memory_order_release);
	}

	void Increment(T val)
	{
		while (locked.test_and_set(std::memory_order_acquire)) {
			;
		}
		value += val;
		locked.clear(std::memory_order_release);
	}

	void Decrement(T val)
	{
		while (locked.test_and_set(std::memory_order_acquire)) {
			;
		}
		value -= val;
		locked.clear(std::memory_order_release);
	}
};

template <typename T>
class ThreadSafeVarVector
{
	std::vector<T> value;
	std::atomic_flag locked = ATOMIC_FLAG_INIT;

public:
	T Get(size_t index)
	{
		while (locked.test_and_set(std::memory_order_acquire)) {
			;
		}
		T val;
		if (value.size() > index)
			val = value[index];
		locked.clear(std::memory_order_release);
		return val;
	}

	void Set(T val, size_t index)
	{
		while (locked.test_and_set(std::memory_order_acquire)) {
			;
		}
		if (value.size() > index)
			value[index] = val;
		locked.clear(std::memory_order_release);
	}

	void Push(T val)
	{
		while (locked.test_and_set(std::memory_order_acquire)) {
			;
		}
		value.push_back(val);
		locked.clear(std::memory_order_release);
	}

	void Pop()
	{
		while (locked.test_and_set(std::memory_order_acquire)) {
			;
		}
		value.pop_back();
		locked.clear(std::memory_order_release);
	}

	size_t Size()
	{
		while (locked.test_and_set(std::memory_order_acquire)) {
			;
		}
		size_t size = value.size();
		locked.clear(std::memory_order_release);
		return size;
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


template <class T>
class Vector;
template <class Y>
class Deque;

namespace std
{
	template <class Y>
	void swap(Vector<Y>& lhs, Vector<Y>& rhs)
	{
		lhs.swap(rhs);
	}
	template <class Y>
	void swap(Deque<Y>& lhs, Deque<Y>& rhs)
	{
		lhs.swap(rhs);
	}
}

class String
{
private:
	std::string* _str = nullptr;

	std::atomic_flag _lock = ATOMIC_FLAG_INIT;

protected:
	void lock()
	{
		while (_lock.test_and_set(std::memory_order_acquire))
#if defined(__cpp_lib_atomic_wait) && __cpp_lib_atomic_wait >= 201907L
			_lock.wait(true, std::memory_order_relaxed)
#endif
				;
	}
	void unlock()
	{
		_lock.clear(std::memory_order_release);
#if defined(__cpp_lib_atomic_wait) && __cpp_lib_atomic_wait >= 201907L
		_lock.notify_one();
#endif
	}

public:
	String() {}
	String(std::string& str)
	{
		_str = new std::string(str);
	}
	String(std::string&& str)
	{
		_str = new std::string(str);
	}
	String(const char* str)
	{
		_str = new std::string(str);
	}
	String(size_t _Count, const char _Ch)
	{
		_str = new std::string(_Count, _Ch);
	}
	// copy assignment
	String& operator=(String& other)
	{
		SpinlockA guard(_lock);
		SpinlockA guardO(other._lock);
		if (this == &other)
			return *this;

		if (_str) {
			delete _str;
			_str = nullptr;
		}
		if (other._str)
			_str = new std::string(*other._str);
		return *this;
	}
	// move assignment
	String& operator=(String&& other) noexcept
	{
		SpinlockA guard(_lock);
		SpinlockA guardO(other._lock);
		if (this == &other)
			return *this;

		if (_str)
			delete _str;
		_str = other._str;
		other._str = nullptr;
		return *this;
	}
	~String()
	{
		SpinlockA guard(_lock);
		if (_str)
			delete _str;
		_str = nullptr;
	}

	void reset()
	{
		SpinlockA guard(_lock);
		if (_str)
			delete _str;
		_str = nullptr;
	}

	size_t size()
	{
		SpinlockA guard(_lock);
		if (_str)
			return _str->size();
		else
			return 0;
	}

	const char* c_str()
	{
		SpinlockA guard(_lock);
		if (_str)
			return _str->c_str();
		else
			return "";
	}

	bool empty()
	{
		SpinlockA guard(_lock);
		if (_str)
			return _str->empty();
		else
			return true;
	}

	String& operator+=(std::string& rhs)
	{
		SpinlockA guard(_lock);
		if (_str) {
			*_str += rhs;
		} else {
			_str = new std::string(rhs);
		}
		return *this;
	}
	String& operator+=(const std::string& rhs)
	{
		SpinlockA guard(_lock);
		if (_str) {
			*_str += rhs;
		} else {
			_str = new std::string(rhs);
		}
		return *this;
	}
	String& operator+=(String& rhs)
	{
		SpinlockA guard(_lock);
		SpinlockA guardO(rhs._lock);
		if (_str) {
			if (rhs._str)
				*_str += *rhs._str;
		} else {
			if (rhs._str)
				_str = new std::string(*rhs._str);
		}
		return *this;
	}

	bool operator<(String& other)
	{
		SpinlockA guard(_lock);
		SpinlockA guardO(other._lock);
		if (_str && other._str)
			return *_str < *other._str;
		else if (_str)
			return true;
		else if (other._str)
			return false;
		else
			return false;
		return true;
	}
	bool operator>(String& rhs) { return rhs < *this; }
	bool operator<=(String& rhs) { return !(*this > rhs); }
	bool operator>=(String& rhs) { return !(*this < rhs); }
	bool operator==(String& rhs)
	{
		SpinlockA guard(_lock);
		SpinlockA guardO(rhs._lock);
		if (_str && rhs._str)
			return *_str == *rhs._str;
		else
			return false;
	}
	bool operator!=(String& rhs) { return !(*this == rhs); }

	char& operator[](std::size_t idx)
	{
		SpinlockA guard(_lock);
		if (_str)
			return (*_str)[idx];
		else
			throw std::out_of_range("string is empty");
	}

	operator std::string()
	{
		SpinlockA guard(_lock);
		if (_str)
			return *_str;
		else
			return std::string();
	}
};

template <class T>
class Vector
{
private:
	std::vector<T>* _vector = nullptr;

	std::atomic_flag _lock = ATOMIC_FLAG_INIT;

protected:
	void lock()
	{
		while (_lock.test_and_set(std::memory_order_acquire))
#if defined(__cpp_lib_atomic_wait) && __cpp_lib_atomic_wait >= 201907L
			_lock.wait(true, std::memory_order_relaxed)
#endif
				;
	}
	void unlock()
	{
		_lock.clear(std::memory_order_release);
#if defined(__cpp_lib_atomic_wait) && __cpp_lib_atomic_wait >= 201907L
		_lock.notify_one();
#endif
	}

public:
	Vector() {}
	Vector(std::vector<T>& vector)
	{
		_vector = new std::vector<T>(vector);
	}
	Vector(std::vector<T>&& vector)
	{
		_vector = new std::vector<T>(vector);
	}
	Vector(size_t _Count, const char _Ch)
	{
		_vector = new std::vector<T>(_Count, _Ch);
	}
	// copy assignment
	Vector& operator=(Vector& other)
	{
		SpinlockA guard(_lock);
		SpinlockA guardO(other._lock);
		if (this == &other)
			return *this;

		if (_vector) {
			delete _vector;
			_vector = nullptr;
		}
		if (other._vector)
			_vector = new std::vector<T>(*other._vector);
		return *this;
	}
	// move assignment
	Vector& operator=(Vector&& other) noexcept
	{
		SpinlockA guard(_lock);
		SpinlockA guardO(other._lock);
		if (this == &other)
			return *this;

		if (_vector)
			delete _vector;
		_vector = other._vector;
		other._vector = nullptr;
		return *this;
	}
	~Vector()
	{
		SpinlockA guard(_lock);
		if (_vector)
			delete _vector;
		_vector = nullptr;
	}

	void reset()
	{
		SpinlockA guard(_lock);
		if (_vector)
			delete _vector;
		_vector = nullptr;
	}

	size_t size()
	{
		SpinlockA guard(_lock);
		if (_vector)
			return _vector->size();
		else
			return 0;
	}

	T* data()
	{
		SpinlockA guard(_lock);
		if (_vector)
			return _vector->data();
		else
			return nullptr;
	}

	bool empty()
	{
		SpinlockA guard(_lock);
		if (_vector)
			return _vector->empty();
		else
			return true;
	}

	void clear()
	{
		SpinlockA guard(_lock);
		if (_vector) {
			delete _vector;
			_vector = nullptr;
		}
	}

	void push_back(T& value)
	{
		SpinlockA guard(_lock);
		if (_vector)
			_vector->push_back(value);
		else {
			_vector = new std::vector<T>();
			_vector->push_back(value);
		}
	}

	void push_back(T&& value)
	{
		SpinlockA guard(_lock);
		if (_vector)
			_vector->push_back(value);
		else {
			_vector = new std::vector<T>();
			_vector->push_back(value);
		}
	}

	std::vector<T>::iterator begin()
	{
		SpinlockA guard(_lock);
		if (_vector)
			return _vector->begin();
		else {
			_vector = new std::vector<T>();
			return _vector->begin();
		}
	}

	std::vector<T>::iterator end()
	{
		SpinlockA guard(_lock);
		if (_vector)
			return _vector->end();
		else {
			_vector = new std::vector<T>();
			return _vector->end();
		}
	}

	void shrink_to_fit()
	{
		SpinlockA guard(_lock);
		if (_vector)
			_vector->shrink_to_fit();
	}

	void swap(std::vector<T>& other)
	{
		SpinlockA guard(_lock);
		if (_vector)
			_vector->swap(other);
		else {
			_vector = new std::vector<T>();
			_vector->swap(other);
		}
	}

	void swap(Vector<T>& other)
	{
		SpinlockA guard(_lock);
		SpinlockA guardO(other._lock);
		if (_vector && other._vector)
			_vector->swap(*other._vector);
		else if (other._vector) {
			_vector = new std::vector<T>();
			_vector->swap(*other._vector);
		} else if (_vector) {
			other._vector = new std::vector<T>();
			_vector->swap(*other._vector);
		}
	}

	void reserve(size_t size)
	{
		SpinlockA guard(_lock);
		if (_vector)
			_vector->reserve(size);
		else {
			_vector = new std::vector<T>();
			_vector->reserve(size);
		}
	}

	T& operator[](std::size_t idx)
	{
		if (_vector)
			return (*_vector)[idx];
		else
			throw std::out_of_range("vector is empty");
	}

	operator std::vector<T>()
	{
		SpinlockA guard(_lock);
		if (_vector)
			return *_vector;
		else
			return std::vector<T>();
	}

	template <class Y>
	friend void std::swap(Vector<Y>& lhs, Vector<Y>& rhs);
};

template <class T>
class Deque
{
private:
	std::deque<T>* _deque = nullptr;

	std::atomic_flag _lock = ATOMIC_FLAG_INIT;

protected:
	void lock()
	{
		while (_lock.test_and_set(std::memory_order_acquire))
#if defined(__cpp_lib_atomic_wait) && __cpp_lib_atomic_wait >= 201907L
			_lock.wait(true, std::memory_order_relaxed)
#endif
				;
	}
	void unlock()
	{
		_lock.clear(std::memory_order_release);
#if defined(__cpp_lib_atomic_wait) && __cpp_lib_atomic_wait >= 201907L
		_lock.notify_one();
#endif
	}

public:
	Deque() {}
	Deque(std::deque<T>& deque)
	{
		_deque = new std::deque<T>(deque);
	}
	Deque(std::deque<T>&& deque)
	{
		_deque = new std::deque<T>(deque);
	}
	Deque(size_t _Count, const char _Ch)
	{
		_deque = new std::deque<T>(_Count, _Ch);
	}
	// copy assignment
	Deque& operator=(Deque& other)
	{
		SpinlockA guard(_lock);
		SpinlockA guardO(other._lock);
		if (this == &other)
			return *this;

		if (_deque) {
			delete _deque;
			_deque = nullptr;
		}
		if (other._deque)
			_deque = new std::deque<T>(*other._deque);
		return *this;
	}
	// move assignment
	Deque& operator=(Deque&& other) noexcept
	{
		SpinlockA guard(_lock);
		SpinlockA guardO(other._lock);
		if (this == &other)
			return *this;

		if (_deque)
			delete _deque;
		_deque = other._deque;
		other._deque = nullptr;
		return *this;
	}
	~Deque()
	{
		SpinlockA guard(_lock);
		if (_deque)
			delete _deque;
		_deque = nullptr;
	}

	void reset()
	{
		SpinlockA guard(_lock);
		if (_deque)
			delete _deque;
		_deque = nullptr;
	}

	size_t size()
	{
		SpinlockA guard(_lock);
		if (_deque)
			return _deque->size();
		else
			return 0;
	}

	T* data()
	{
		SpinlockA guard(_lock);
		if (_deque)
			return _deque->data();
		else
			return nullptr;
	}

	bool empty()
	{
		SpinlockA guard(_lock);
		if (_deque)
			return _deque->empty();
		else
			return true;
	}

	void clear()
	{
		SpinlockA guard(_lock);
		if (_deque) {
			delete _deque;
			_deque = nullptr;
		}
	}

	void push_back(T& value)
	{
		SpinlockA guard(_lock);
		if (_deque)
			_deque->push_back(value);
		else {
			_deque = new std::deque<T>();
			_deque->push_back(value);
		}
	}

	void push_back(T&& value)
	{
		SpinlockA guard(_lock);
		if (_deque)
			_deque->push_back(value);
		else {
			_deque = new std::deque<T>();
			_deque->push_back(value);
		}
	}

	std::deque<T>::iterator begin()
	{
		SpinlockA guard(_lock);
		if (_deque)
			return _deque->begin();
		else {
			_deque = new std::deque<T>();
			return _deque->begin();
		}
	}

	std::deque<T>::iterator end()
	{
		SpinlockA guard(_lock);
		if (_deque)
			return _deque->end();
		else {
			_deque = new std::deque<T>();
			return _deque->end();
		}
	}

	void shrink_to_fit()
	{
		SpinlockA guard(_lock);
		if (_deque)
			_deque->shrink_to_fit();
	}

	void swap(std::deque<T>& other)
	{
		SpinlockA guard(_lock);
		if (_deque)
			_deque->swap(other);
		else {
			_deque = new std::deque<T>();
			_deque->swap(other);
		}
	}

	void swap(Deque<T>& other)
	{
		SpinlockA guard(_lock);
		SpinlockA guardO(other._lock);
		if (_deque && other._deque)
			_deque->swap(*other._deque);
		else if (other._deque) {
			_deque = new std::deque<T>();
			_deque->swap(*other._deque);
		} else if (_deque) {
			other._deque = new std::deque<T>();
			_deque->swap(*other._deque);
		}
	}

	void reserve(size_t size)
	{
		SpinlockA guard(_lock);
		if (_deque)
			_deque->reserve(size);
		else {
			_deque = new std::deque<T>();
			_deque->reserve(size);
		}
	}

	T& operator[](std::size_t idx)
	{
		if (_deque)
			return (*_deque)[idx];
		else
			throw std::out_of_range("deque is empty");
	}

	operator std::deque<T>()
	{
		SpinlockA guard(_lock);
		if (_deque)
			return *_deque;
		else
			return std::deque<T>();
	}

	template <class Y>
	friend void std::swap(Deque<Y>& lhs, Deque<Y>& rhs);
};
