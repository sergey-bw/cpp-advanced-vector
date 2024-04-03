#pragma once
#include <cassert>
#include <cstdlib>
#include <new>
#include <utility>
#include <memory>

template <typename T>
class RawMemory {
public:
    RawMemory() = default;

    explicit RawMemory(size_t capacity);
    RawMemory(const RawMemory&) = delete;
    RawMemory& operator=(const RawMemory& rhs) = delete;
    RawMemory(RawMemory&& other) noexcept;
    RawMemory& operator=(RawMemory&& rhs) noexcept;
    ~RawMemory();

    T* operator+(size_t offset) noexcept;
    const T* operator+(size_t offset) const noexcept;
    const T& operator[](size_t index) const noexcept;
    T& operator[](size_t index) noexcept;

    void Swap(RawMemory& other) noexcept;

    const T* GetAddress() const noexcept;
    T* GetAddress() noexcept;

    size_t Capacity() const;

private:
    // Выделяет сырую память под n элементов и возвращает указатель на неё
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    // Освобождает сырую память, выделенную ранее по адресу buf при помощи Allocate
    static void Deallocate(T* buf) noexcept {
        operator delete(buf);
    }

    T* buffer_ = nullptr;
    size_t capacity_ = 0;
};

template <typename T>
class Vector { 
public:
    using iterator = T*;
    using const_iterator = const T*;
    
    iterator begin() noexcept;
    iterator end() noexcept;
    const_iterator begin() const noexcept;
    const_iterator end() const noexcept;
    const_iterator cbegin() const noexcept;
    const_iterator cend() const noexcept;

    Vector() = default;
    explicit Vector(size_t size);
    Vector(const Vector& other);
    Vector(Vector&& other) noexcept;
    Vector& operator=(const Vector& rhs);
    Vector& operator=(Vector&& rhs) noexcept;
    ~Vector();

    void Swap(Vector& other) noexcept;

    template <typename... Args>
    T& EmplaceBack(Args&&... args);
    void PushBack(const T& new_item);
    void PushBack(T&& new_item);
    void PopBack() noexcept;

    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args);
    iterator Insert(const_iterator pos, const T& value);
    iterator Insert(const_iterator pos, T&& value);
    iterator Erase(const_iterator pos) /*noexcept(std::is_nothrow_move_assignable_v<T>)*/;

    void Resize(size_t new_size);
    void Reserve(size_t new_capacity);
    size_t Size() const noexcept;
    size_t Capacity() const noexcept;

    const T& operator[](size_t index) const noexcept;
    T& operator[](size_t index) noexcept;

private:
    RawMemory<T> data_;
    size_t size_ = 0;
    
    void UninitializedCopyOrMove(iterator it_from, size_t item_count, iterator it_to);
};

/**
 * Class Vector<T>
*/

template <typename T>
RawMemory<T>::RawMemory(size_t capacity)
    : buffer_(Allocate(capacity))
    , capacity_(capacity) {
}

template <typename T>
RawMemory<T>::RawMemory(RawMemory&& other) noexcept {
    // Используем оператор перемещиния
    *this = std::move(other);
}

template <typename T>
typename RawMemory<T>::RawMemory& RawMemory<T>::operator=(RawMemory&& rhs) noexcept {
    if (this != &rhs) {
        Swap(rhs);
    }
    return *this;
}

template <typename T>
RawMemory<T>::~RawMemory() {
    Deallocate(buffer_);
}

template <typename T>
T* RawMemory<T>::operator+(size_t offset) noexcept {
    // Разрешается получать адрес ячейки памяти, следующей за последним элементом массива
    assert(offset <= capacity_);
    return buffer_ + offset;
}

template <typename T>
const T* RawMemory<T>::operator+(size_t offset) const noexcept {
    return const_cast<RawMemory&>(*this) + offset;
}

template <typename T>
const T& RawMemory<T>::operator[](size_t index) const noexcept {
    return const_cast<RawMemory&>(*this)[index];
}

template <typename T>
T& RawMemory<T>::operator[](size_t index) noexcept {
    assert(index < capacity_);
    return buffer_[index];
}

template <typename T>
void RawMemory<T>::Swap(RawMemory& other) noexcept {
    std::swap(buffer_, other.buffer_);
    std::swap(capacity_, other.capacity_);
}

template <typename T>
const T* RawMemory<T>::GetAddress() const noexcept {
    return buffer_;
}

template <typename T>
T* RawMemory<T>::GetAddress() noexcept {
    return buffer_;
}

template <typename T>
size_t RawMemory<T>::Capacity() const {
    return capacity_;
}

/**
 * Class Vector<T>
*/

template <typename T>
typename Vector<T>::iterator Vector<T>::begin() noexcept {
    return data_.GetAddress();
}

template <typename T>
typename Vector<T>::iterator Vector<T>::end() noexcept {
    return data_.GetAddress() + size_;
}

template <typename T>
typename Vector<T>::const_iterator Vector<T>::begin() const noexcept {
    return cbegin();
}

template <typename T>
typename Vector<T>::const_iterator Vector<T>::end() const noexcept {
    return cend();
}

template <typename T>
typename Vector<T>::const_iterator Vector<T>::cbegin() const noexcept {
    return data_.GetAddress();
}

template <typename T>
typename Vector<T>::const_iterator Vector<T>::cend() const noexcept {
    return data_.GetAddress() + size_;
}

template <typename T>
Vector<T>::Vector(size_t size)
    : data_(size)
    , size_(size)
{
    std::uninitialized_value_construct_n(begin(), size);
}

template <typename T>
Vector<T>::Vector(const Vector& other)
    : data_(other.size_)
    , size_(other.size_)
{
    std::uninitialized_copy_n(other.begin(), other.size_, begin());
}

template <typename T>
Vector<T>::Vector(Vector&& other) noexcept {
    // Используем оператор перемещиния
    *this = std::move(other);
}

template <typename T>
typename Vector<T>::Vector& Vector<T>::operator=(const Vector& rhs) {
    if (this != &rhs) {
        if (rhs.size_ <= data_.Capacity()) {
            if (size_ >= rhs.size_) {
                // Копируем и удаляем лишние
                for (size_t i = 0; i < rhs.size_; ++i) {
                    data_[i] = rhs.data_[i];
                }
                std::destroy_n(begin() + rhs.size_, size_ - rhs.size_);
            } else {
                // Копируем и создаем недостающие
                for (size_t i = 0; i < size_; ++i) {
                    data_[i] = rhs.data_[i];
                }
                std::uninitialized_copy_n(rhs.begin() + size_, rhs.size_ - size_, end());
            }
            size_ = rhs.size_;
        } else {
            // copy-and-swap
            Vector rhs_copy(rhs);
            Swap(rhs_copy);
        }
    }
    return *this;
}

template <typename T>
typename Vector<T>::Vector& Vector<T>::operator=(Vector&& rhs) noexcept {
    Swap(rhs);
    return *this;
}

template <typename T>
Vector<T>::~Vector() {
    std::destroy_n(begin(), size_);
}

template <typename T>
void Vector<T>::Swap(Vector& other) noexcept {
    std::swap(data_, other.data_);
    std::swap(size_, other.size_);
}

template <typename T>
template <typename... Args>
T& Vector<T>::EmplaceBack(Args&&... args) {
    if (size_ == data_.Capacity()) {
        RawMemory<T> new_data(std::max(static_cast<size_t>(1), data_.Capacity() * 2));
        new (new_data.GetAddress() + size_) T(std::forward<Args>(args)...);
        UninitializedCopyOrMove(begin(), size_, new_data.GetAddress());
        std::destroy_n(begin(), size_);
        data_.Swap(new_data);
    } else {
        new (end()) T(std::forward<Args>(args)...);
    }
    ++size_;
    return data_[size_ - 1];
}

template <typename T>
template <typename... Args>
typename Vector<T>::iterator Vector<T>::Emplace(const_iterator pos, Args&&... args) {
    if (pos < begin() || pos > end()) {
        throw;
    }

    size_t index = std::distance(cbegin(), pos);
    if (index != size_) {
        if (size_ == data_.Capacity()) { // Вставка в заполненый вектор
            RawMemory<T> new_data(std::max(static_cast<size_t>(1), data_.Capacity() * 2));
            new (new_data.GetAddress() + index) T(std::forward<Args>(args)...);

            try {
                UninitializedCopyOrMove(begin(), index, new_data.GetAddress());
            } catch (...) {
                std::destroy_at(new_data.GetAddress() + index);
                throw;
            }

            try {
                UninitializedCopyOrMove(begin() + index, size_ - index, new_data.GetAddress() + index + 1);
            } catch (...) {
                std::destroy_n(new_data.GetAddress(), index + 1);
                throw;
            }

            std::destroy_n(begin(), size_);
            data_.Swap(new_data);
        } else {
            T tmp_T(std::forward<Args>(args)...); // Временный объект
            new (data_ + size_) T(std::move(data_[size_ - 1]));
            std::move_backward(begin() + index, end() - 1, end());
            data_[index] = std::move(tmp_T);
        }
        ++size_;
    } else { // Вставка в конец вектора
        EmplaceBack(std::forward<Args>(args)...);
    }
    return begin() + index;
}

template <typename T>
void Vector<T>::PushBack(const T& new_item) {
    EmplaceBack(new_item);
}

template <typename T>
void Vector<T>::PushBack(T&& new_item) {
    EmplaceBack(std::move(new_item));
}

template <typename T>
void Vector<T>::PopBack() noexcept {
    std::destroy_at(end() - 1);
    --size_;
}

template <typename T>
typename Vector<T>::iterator Vector<T>::Insert(const_iterator pos, const T& value) {
    return Emplace(pos, value);
}

template <typename T>
typename Vector<T>::iterator Vector<T>::Insert(const_iterator pos, T&& value) {
    return Emplace(pos, std::move(value));
}

template <typename T>
typename Vector<T>::iterator Vector<T>::Erase(const_iterator pos) /*noexcept(std::is_nothrow_move_assignable_v<T>)*/ {
    if (pos < begin() || pos > end()) {
        throw;
    }
    size_t index = std::distance(cbegin(), pos);
    if (index == size_) { // Удаление последнего элемента
        PopBack();
    } else {
        std::move(begin() + index + 1, end(), begin() + index);
        --size_;
        std::destroy_n(end(), 1);
    }

    return begin() + index;
}

template <typename T>
void Vector<T>::Resize(size_t new_size) {
    if (new_size > size_) {
        Reserve(new_size);
        std::uninitialized_value_construct_n(end(), new_size - size_);
    } else {
        std::destroy_n(begin() + new_size, size_ - new_size);
    }
    size_ = new_size;
}

template <typename T>
void Vector<T>::Reserve(size_t new_capacity) {
    if (new_capacity <= data_.Capacity()) {
        return;
    }

    RawMemory<T> new_data(new_capacity);
    UninitializedCopyOrMove(begin(), size_, new_data.GetAddress());
    std::destroy_n(begin(), size_);
    data_.Swap(new_data);
}

template <typename T>
size_t Vector<T>::Size() const noexcept {
    return size_;
}

template <typename T>
size_t Vector<T>::Capacity() const noexcept {
    return data_.Capacity();
}

template <typename T>
const T& Vector<T>::operator[](size_t index) const noexcept {
    return const_cast<Vector&>(*this)[index];
}

template <typename T>
T& Vector<T>::operator[](size_t index) noexcept {
    assert(index < size_);
    return data_[index];
}

template <typename T>
void Vector<T>::UninitializedCopyOrMove(iterator it_from, size_t item_count, iterator it_to) {
    if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
        std::uninitialized_move_n(it_from, item_count, it_to);
    } else {
        std::uninitialized_copy_n(it_from, item_count, it_to);
    }
}