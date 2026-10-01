#pragma once

#include <cstddef>
#include <stdexcept>
#include <utility>

// динамический массив с удвоением вместимости
// для хранения типа нужен конструктор по умолчанию
template <typename T>
class DynamicArray {
private:
    T* data_;
    size_t size_;
    size_t capacity_;

    // выделяет новый буфер и переносит в него элементы
    void Reallocate(size_t newCapacity) {
        T* newData = new T[newCapacity];
        for (size_t i = 0; i < size_; ++i) {
            newData[i] = std::move(data_[i]);
        }
        delete[] data_;
        data_ = newData;
        capacity_ = newCapacity;
    }

    void Swap(DynamicArray& other) noexcept {
        std::swap(data_, other.data_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

public:
    DynamicArray() : data_(nullptr), size_(0), capacity_(0) {}

    // массив из count одинаковых элементов
    DynamicArray(size_t count, const T& value)
            : data_(count > 0 ? new T[count] : nullptr), size_(count), capacity_(count) {
        for (size_t i = 0; i < size_; ++i) {
            data_[i] = value;
        }
    }

    DynamicArray(const DynamicArray& other)
            : data_(other.capacity_ > 0 ? new T[other.capacity_] : nullptr),
              size_(other.size_),
              capacity_(other.capacity_) {
        for (size_t i = 0; i < size_; ++i) {
            data_[i] = other.data_[i];
        }
    }

    // забираем буфер у другого массива и оставляем его пустым
    DynamicArray(DynamicArray&& other) noexcept
            : data_(other.data_), size_(other.size_), capacity_(other.capacity_) {
        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
    }

    DynamicArray& operator=(const DynamicArray& other) {
        if (this != &other) {
            DynamicArray tmp(other);
            Swap(tmp);
        }
        return *this;
    }

    DynamicArray& operator=(DynamicArray&& other) noexcept {
        if (this != &other) {
            delete[] data_;
            data_ = other.data_;
            size_ = other.size_;
            capacity_ = other.capacity_;
            other.data_ = nullptr;
            other.size_ = 0;
            other.capacity_ = 0;
        }
        return *this;
    }

    ~DynamicArray() {
        delete[] data_;
    }

    size_t GetSize() const { return size_; }

    size_t GetCapacity() const { return capacity_; }

    bool IsEmpty() const { return size_ == 0; }

    T& Get(size_t index) {
        if (index >= size_) {
            throw std::out_of_range("DynamicArray::Get index out of range");
        }
        return data_[index];
    }

    const T& Get(size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("DynamicArray::Get index out of range");
        }
        return data_[index];
    }

    T& operator[](size_t index) { return Get(index); }

    const T& operator[](size_t index) const { return Get(index); }

    void PushBack(const T& value) {
        if (size_ == capacity_) {
            // value может лежать внутри самого массива поэтому копируем до перевыделения
            T copy = value;
            Reallocate(capacity_ == 0 ? 4 : capacity_ * 2);
            data_[size_++] = std::move(copy);
        } else {
            data_[size_++] = value;
        }
    }

    void PopBack() {
        if (size_ == 0) {
            throw std::out_of_range("DynamicArray::PopBack array is empty");
        }
        --size_;
    }

    // буфер остается выделенным
    void Clear() {
        size_ = 0;
    }
};