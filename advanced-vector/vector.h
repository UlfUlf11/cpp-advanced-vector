#pragma once

#include <cassert>
#include <memory>
#include <algorithm>
#include <cstdlib>
#include <new>
#include <utility>
#include <iterator>

//Отдельный класс-обёртка, управляющий сырой памятью.
/*
Шаблонный класс RawMemory будет отвечать за хранение буфера, который вмещает заданное количество элементов, и предоставлять доступ к элементам по индексу:
*/
template <typename T>
class RawMemory
{
public:
    RawMemory() = default;

    explicit RawMemory(size_t capacity)
        : buffer_(Allocate(capacity))
        , capacity_(capacity)
    {
    }

    ~RawMemory()
    {
        Deallocate(buffer_);
    }


    /*
    Операция копирования не имеет смысла для класса RawMemory, так как у него нет информации о количестве находящихся в сырой памяти элементов. Поэтому конструктор копирования и копирующий оператор присваивания в классе RawMemory должны быть запрещены.
    */
    RawMemory(const RawMemory&) = delete;
    RawMemory& operator=(const RawMemory& rhs) = delete;

    RawMemory(RawMemory&& other) noexcept
        : buffer_(std::exchange(other.buffer_, nullptr))
        , capacity_(std::exchange(other.capacity_, 0))
    {

    }

    RawMemory& operator=(RawMemory&& rhs) noexcept
    {

        if (this != &rhs)
        {
            buffer_ = std::move(rhs.buffer_);
            capacity_ = std::move(rhs.capacity_);
            rhs.buffer_ = nullptr;
            rhs.capacity_ = 0;
        }

        return *this;
    }

    T* operator+(size_t offset) noexcept
    {
        assert(offset <= capacity_);
        return buffer_ + offset;
    }
    const T* operator+(size_t offset) const noexcept
    {
        return const_cast<RawMemory&>(*this) + offset;
    }

    const T& operator[](size_t index) const noexcept
    {
        return const_cast<RawMemory&>(*this)[index];
    }
    T& operator[](size_t index) noexcept
    {
        assert(index < capacity_);
        return buffer_[index];
    }

    void Swap(RawMemory& other) noexcept
    {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

    const T* GetAddress() const noexcept
    {
        return buffer_;
    }
    T* GetAddress() noexcept
    {
        return buffer_;
    }
    size_t Capacity() const
    {
        return capacity_;
    }

private:
    // Выделяет сырую память под n элементов и возвращает указатель на неё
    static T* Allocate(size_t n)
    {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    // Освобождает сырую память, выделенную ранее по адресу buf при помощи Allocate
    static void Deallocate(T* buf) noexcept
    {
        operator delete(buf);
    }

    T* buffer_ = nullptr;
    size_t capacity_ = 0;
};

template <typename T>
class Vector
{
public:

    using iterator = T*;
    using const_iterator = const T*;

    iterator begin() noexcept
    {
        return data_.GetAddress();
    }

    iterator end() noexcept
    {
        return size_ + data_.GetAddress();
    }

    const_iterator begin() const noexcept
    {
        return cbegin();
    }

    const_iterator end() const noexcept
    {
        return cend();
    }

    const_iterator cbegin() const noexcept
    {
        return data_.GetAddress();
    }

    const_iterator cend() const noexcept
    {
        return size_ + data_.GetAddress();
    }

    Vector() = default;

    explicit Vector(size_t size)
        : data_(size)
        , size_(size)
    {
        std::uninitialized_value_construct_n(data_.GetAddress(), size);
    }

    Vector(const Vector& other)
        : data_(other.size_)
        , size_(other.size_)
    {
        std::uninitialized_copy_n(other.data_.GetAddress(), size_, data_.GetAddress());
    }

    Vector(Vector&& other) noexcept
        : data_(std::move(other.data_))
        , size_(std::exchange(other.size_, 0))
    {

    }

    ~Vector()
    {
        std::destroy_n(data_.GetAddress(), size_);
    }

    size_t Size() const noexcept
    {
        return size_;
    }

    size_t Capacity() const noexcept
    {
        return data_.Capacity();
    }

    void Swap(Vector& other) noexcept
    {
        data_.Swap(other.data_), std::swap(size_, other.size_);
    }

    void Reserve(size_t new_capacity)
    {
        if (new_capacity <= data_.Capacity())
        {
            return;
        }
        RawMemory<T> new_data(new_capacity);

        // constexpr оператор if будет вычислен во время компиляции
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>)
        {
            std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        else
        {
            std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
        }

        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(new_data);
    }

    void Resize(size_t new_size)
    {
        if (new_size < size_)
        {
            std::destroy_n(data_.GetAddress() + new_size, size_ - new_size);
        }
        else
        {
            if (new_size > data_.Capacity())
            {
                const size_t new_capacity = std::max(data_.Capacity() * 2, new_size);
                Reserve(new_capacity);
            }
            std::uninitialized_value_construct_n(data_.GetAddress() + size_, new_size - size_);
        }
        size_ = new_size;
    }

    void PushBack(const T& value)
    {
        if (size_ == Capacity())
        {
            RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
            std::uninitialized_copy_n(&value, 1, new_data.GetAddress() + size_);
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>)
            {
                std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            else
            {
                std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(new_data);
        }
        else
        {
            new (data_ + size_) T(value);
        }
        ++size_;
    }

    void PushBack(T&& value)
    {
        if (size_ == Capacity())
        {
            RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
            std::uninitialized_move_n(&value, 1, new_data.GetAddress() + size_);
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>)
            {
                std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            else
            {
                std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(new_data);
        }
        else
        {
            new (data_ + size_) T(std::move(value));
        }
        ++size_;
    }

    void PopBack() noexcept
    {
        if (size_ > 0)
        {
            std::destroy_at(data_.GetAddress() + size_ - 1);
            --size_;
        }
    }

    Vector& operator=(const Vector& other)
    {
        if (this != &other)
        {
            if (other.size_ <= data_.Capacity())
            {
                if (size_ <= other.size_)
                {

                    std::copy(other.data_.GetAddress(),
                        other.data_.GetAddress() + size_,
                        data_.GetAddress());

                    std::uninitialized_copy_n(other.data_.GetAddress() + size_,
                        other.size_ - size_,
                        data_.GetAddress() + size_);
                }
                else
                {
                    std::copy(other.data_.GetAddress(),
                        other.data_.GetAddress() + other.size_,
                        data_.GetAddress());

                    std::destroy_n(data_.GetAddress() + other.size_,
                        size_ - other.size_);
                }

                size_ = other.size_;

            }
            else
            {
                Vector other_copy(other);
                Swap(other_copy);
            }
        }

        return *this;
    }

    Vector& operator = (Vector&& other) noexcept
    {
        Swap(other);
        return *this;
    }

    /*
    В константном операторе [] используется оператор  const_cast, чтобы снять константность с ссылки на текущий объект и вызвать неконстантную версию оператора []. Так получится избавиться от дублирования проверки assert(index < size). Оператор const_cast позволяет сделать то, что нельзя, но, если очень хочется, можно. В данном случае нельзя вызвать неконстантный метод из константного. Но неконстантный оператор [] тут не модифицирует состояние объекта, поэтому его можно вызвать, предварительно сняв константность с объекта.
    */
    const T& operator[](size_t index) const noexcept
    {
        return const_cast<Vector&>(*this)[index];
    }

    T& operator[](size_t index) noexcept
    {
        assert(index < size_);
        return data_[index];
    }


    template <typename... Args>
    T& EmplaceBack(Args&&... args)
    {
        T* result = nullptr;
        if (size_ == Capacity())
        {
            RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
            result = new (new_data + size_) T(std::forward<Args>(args)...);
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>)
            {
                std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            else
            {
                try
                {
                    std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
                }
                catch (...)
                {
                    std::destroy_n(new_data.GetAddress() + size_, 1);
                    throw;
                }
            }
            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(new_data);
        }
        else
        {
            result = new (data_ + size_) T(std::forward<Args>(args)...);
        }
        ++size_;
        return *result;
    }

    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args)
    {
        assert(pos >= begin() && pos <= end());
        size_t shift = pos - begin();
        iterator result = nullptr;
        if (size_ == Capacity())
        {
            RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
            result = new (new_data + shift) T(std::forward<Args>(args)...);
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>)
            {
                std::uninitialized_move_n(data_.GetAddress(), shift, new_data.GetAddress());
                std::uninitialized_move_n(data_.GetAddress() + shift, size_ - shift, new_data.GetAddress() + shift + 1);
            }
            else
            {
                try
                {
                    std::uninitialized_copy_n(data_.GetAddress(), shift, new_data.GetAddress());
                    std::uninitialized_copy_n(data_.GetAddress() + shift, size_ - shift, new_data.GetAddress() + shift + 1);
                }
                catch (...)
                {
                    std::destroy_n(new_data.GetAddress() + shift, 1);
                    throw;
                }
            }
            std::destroy_n(begin(), size_);
            data_.Swap(new_data);
        }
        else
        {
            if (size_ != 0)
            {
                new (data_ + size_) T(std::move(*(data_.GetAddress() + size_ - 1)));
                try
                {
                    std::move_backward(begin() + shift,
                        data_.GetAddress() + size_,
                        data_.GetAddress() + size_ + 1);
                }
                catch (...)
                {
                    std::destroy_n(data_.GetAddress() + size_, 1);
                    throw;
                }
                std::destroy_at(begin() + shift);
            }
            result = new (data_ + shift) T(std::forward<Args>(args)...);
        }
        ++size_;
        return result;
    }


    iterator Erase(const_iterator pos) noexcept(std::is_nothrow_move_assignable_v<T>)
    {
        assert(pos >= begin() && pos < end());
        size_t shift = pos - begin();
        std::move(begin() + shift + 1, end(), begin() + shift);
        PopBack();
        return begin() + shift;
    }

    iterator Insert(const_iterator pos, const T& value)
    {
        return Emplace(pos, value);
    }

    iterator Insert(const_iterator pos, T&& value)
    {
        return Emplace(pos, std::move(value));
    }

private:
    RawMemory<T> data_;
    size_t size_ = 0;
};

