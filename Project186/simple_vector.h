#pragma once

#include <cassert>
#include <initializer_list>
#include <algorithm>
#include <stdexcept>
#include <iterator>
#include <utility>

#include "array_ptr.h"

class ReserveProxyObj {
	size_t capacity_;
public:
	explicit ReserveProxyObj(size_t capacity) : capacity_(capacity) {}
	size_t GetCapacity() {
		return capacity_;
	}
};

template <typename Type>
class SimpleVector {

	ArrayPtr<Type> items_;
	size_t size_;
	size_t capacity_;

public:
	using Iterator = Type*;
	using ConstIterator = const Type*;

	SimpleVector() noexcept : items_(), size_(0), capacity_(0) {};

	// Создаёт вектор из size элементов, инициализированных значением value
	SimpleVector(size_t size, const Type& value) : items_(size), size_(size), capacity_(size) {
		std::fill(items_.Get(), items_.Get() + size, value);
	}

	// Создаёт вектор из size элементов, инициализированных значением по умолчанию
	explicit SimpleVector(size_t size) : SimpleVector(size, Type()) {}

	explicit SimpleVector(ReserveProxyObj po) : items_{ po.GetCapacity() }, size_(0), capacity_{ po.GetCapacity() } {}

	// Создаёт вектор из std::initializer_list
	SimpleVector(std::initializer_list<Type> init) : items_{ init.size() }, size_{ init.size() }, capacity_{ init.size() } {
		std::copy(init.begin(), init.end(), items_.Get());
	}

	SimpleVector(const SimpleVector& other) : items_{ other.capacity_ }, size_{ other.size_ }, capacity_{ other.capacity_ } {
		std::copy(other.begin(), other.end(), items_.Get());
	}

	SimpleVector(SimpleVector&& other) noexcept : items_(std::move(other.items_)), size_(other.size_), capacity_(other.capacity_) {
		other.size_ = 0;
		other.capacity_ = 0;
	}

	SimpleVector& operator=(const SimpleVector& rhs) {
		if (this == &rhs) {
			return *this;
		}
		ArrayPtr<Type> tmp(rhs.capacity_);
		std::copy(rhs.begin(), rhs.begin() + rhs.size_, tmp.Get());
		items_.swap(tmp);
		size_ = rhs.size_;
		capacity_ = rhs.capacity_;
		return *this;
	}

	void Reserve(size_t new_capacity) {
		if (new_capacity > capacity_) {
			ArrayPtr<Type> tmp(new_capacity);
			std::copy(items_.Get(), items_.Get() + size_, tmp.Get());
			items_.swap(tmp);
			capacity_ = new_capacity;
		}
	}

	// Добавляет элемент в конец вектора
	// При нехватке места увеличивает вдвое вместимость вектора
	void PushBack(const Type& item) {
		incrementSize();
		items_[size_ - 1] = item;
	}

	void PushBack(Type&& item) {
		incrementSize();
		items_[size_ - 1] = std::move(item);
	}

	// Вставляет значение value в позицию pos.
	// Возвращает итератор на вставленное значение
	// Если перед вставкой значения вектор был заполнен полностью,
	// вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
	Iterator Insert(ConstIterator pos, const Type& value) {
		size_t dist = std::distance(cbegin(), pos);
		Iterator it = begin() + dist;
		if (capacity_ == 0) {
			size_t new_capacity = 1;
			ArrayPtr<Type> tmp(new_capacity);
			*tmp.Get() = value;
			items_.swap(tmp);
			++size_;
			capacity_ = new_capacity;
			return items_.Get();
		}
		if (size_ >= capacity_) {
			size_t new_capacity = capacity_ * 2;
			ArrayPtr<Type> tmp(new_capacity);
			std::copy(items_.Get(), it, tmp.Get());
			std::copy(pos, cend(), tmp.Get() + dist + 1);
			*(tmp.Get() + dist) = value;
			items_.swap(tmp);
			++size_;
			capacity_ = new_capacity;
			return items_.Get() + dist;
		}
		else {
			std::copy(pos, cend(), it + 1);
			*(it) = value;
			++size_;
			return it;
		}
	}

	Iterator Insert(Iterator pos, Type&& value) {
		size_t dist = std::distance(begin(), pos);
		Iterator it = begin() + dist;
		if (capacity_ == 0) {
			size_t new_capacity = 1;
			ArrayPtr<Type> tmp(new_capacity);
			*tmp.Get() = std::move(value);
			items_.swap(tmp);
			++size_;
			capacity_ = new_capacity;
			return items_.Get();
		}
		if (size_ >= capacity_) {
			size_t new_capacity = capacity_ * 2;
			ArrayPtr<Type> tmp(new_capacity);
			std::copy(std::make_move_iterator(items_.Get()), std::make_move_iterator(it), tmp.Get());
			std::copy(std::make_move_iterator(pos), std::make_move_iterator(end()), tmp.Get() + dist + 1);
			*(tmp.Get() + dist) = std::move(value);
			items_.swap(tmp);
			++size_;
			capacity_ = new_capacity;
			return items_.Get() + dist;
		}
		else {
			std::copy(std::make_move_iterator(pos), std::make_move_iterator(end()), it + 1);
			*(it) = std::move(value);
			++size_;
			return it;
		}
	}

	// "Удаляет" последний элемент вектора. Вектор не должен быть пустым
	void PopBack() noexcept {
		--size_;
	}

	// Удаляет элемент вектора в указанной позиции
	Iterator Erase(ConstIterator pos) {
		size_t dist = std::distance(cbegin(), pos);
		Iterator it = begin() + dist;
		std::copy(pos + 1, cend(), it);
		--size_;
		return begin() + dist;
	}

	Iterator Erase(Iterator pos) {
		size_t dist = std::distance(begin(), pos);
		Iterator it = begin() + dist;
		std::copy(std::make_move_iterator(pos + 1), std::make_move_iterator(end()), it);
		--size_;
		return begin() + dist;
	}

	// Обменивает значение с другим вектором
	void swap(SimpleVector& other) noexcept {
		size_t tempSize_ = other.size_;
		size_t tempCapacity_ = other.capacity_;
		other.capacity_ = capacity_;
		other.size_ = size_;
		items_.swap(other.items_);
		capacity_ = tempCapacity_;
		size_ = tempSize_;
	}

	// Возвращает количество элементов в массиве
	size_t GetSize() const noexcept {
		return size_;
	}

	// Возвращает вместимость массива
	size_t GetCapacity() const noexcept {
		return capacity_;
	}

	// Сообщает, пустой ли массив
	bool IsEmpty() const noexcept {
		return size_ == 0;
	}

	// Возвращает ссылку на элемент с индексом index
	Type& operator[](size_t index) noexcept {
		return items_[index];
	}

	// Возвращает константную ссылку на элемент с индексом index
	const Type& operator[](size_t index) const noexcept {
		return items_[index];
	}

	// Возвращает константную ссылку на элемент с индексом index
	// Выбрасывает исключение std::out_of_range, если index >= size
	Type& At(size_t index) {
		if (index >= size_) {
			throw std::out_of_range("");
		}
		return items_[index];
	}

	// Возвращает константную ссылку на элемент с индексом index
	// Выбрасывает исключение std::out_of_range, если index >= size
	const Type& At(size_t index) const {
		if (index >= size_) {
			throw std::out_of_range("");
		}
		return items_[index];
	}

	// Обнуляет размер массива, не изменяя его вместимость
	void Clear() noexcept {
		size_ = 0;
	}

	// Изменяет размер массива.
	// При увеличении размера новые элементы получают значение по умолчанию для типа Type
	void Resize(size_t new_size) {
		if (new_size == size_) { return; }
		if (new_size > capacity_) {
			ArrayPtr<Type> tmp(new_size);
			std::copy(std::make_move_iterator(items_.Get()), std::make_move_iterator(items_.Get()) + size_, tmp.Get());
			std::fill(tmp.Get() + size_, tmp.Get() + new_size, Type());
			items_.swap(tmp);
			size_ = new_size;
			capacity_ = new_size;
		}
		else {
			if (new_size > size_) {
				std::fill(items_.Get() + size_, items_.Get() + new_size, Type());
			}
			size_ = new_size;
		}
	}

	// Возвращает итератор на начало массива
	// Для пустого массива может быть равен (или не равен) nullptr
	Iterator begin() noexcept {
		return items_.Get();
	}

	// Возвращает итератор на элемент, следующий за последним
	// Для пустого массива может быть равен (или не равен) nullptr
	Iterator end() noexcept {
		return items_.Get() + size_;
	}

	// Возвращает константный итератор на начало массива
	// Для пустого массива может быть равен (или не равен) nullptr
	ConstIterator begin() const noexcept {
		return items_.Get();
	}

	// Возвращает итератор на элемент, следующий за последним
	// Для пустого массива может быть равен (или не равен) nullptr
	ConstIterator end() const noexcept {
		return items_.Get() + size_;
	}

	// Возвращает константный итератор на начало массива
	// Для пустого массива может быть равен (или не равен) nullptr
	ConstIterator cbegin() const noexcept {
		return items_.Get();
	}

	// Возвращает итератор на элемент, следующий за последним
	// Для пустого массива может быть равен (или не равен) nullptr
	ConstIterator cend() const noexcept {
		return items_.Get() + size_;
	}

private:
	// return true if reallocation is occur
	bool incrementSize() {
		if (size_ >= capacity_) {
			if (capacity_ == 0) {
				ArrayPtr<Type> tmp(1);
				items_.swap(tmp);
				size_ = 1;
				capacity_ = 1;
			}
			else {
				size_t new_capacity = capacity_ * 2;
				ArrayPtr<Type> tmp(new_capacity);
				std::copy(std::make_move_iterator(items_.Get()), std::make_move_iterator(items_.Get() + size_), tmp.Get());
				items_.swap(tmp);
				++size_;
				capacity_ = new_capacity;
			}
			return true;
		}
		else {
			++size_;
			return false;
		}
	}
};

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
	if (lhs.GetSize() != rhs.GetSize()) {
		return false;
	}
	for (size_t i = 0; i < lhs.GetSize(); ++i) {
		if (lhs.At(i) != rhs.At(i)) {
			return false;
		}
	}
	return true;
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
	return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
	return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
	return !(rhs < lhs);
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
	return (rhs < lhs);
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
	return !(lhs < rhs);
}

ReserveProxyObj Reserve(size_t capacity_to_reserve) {
	return ReserveProxyObj(capacity_to_reserve);
};