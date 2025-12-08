#pragma once
#include <memory>
#include <iostream>
#include <cassert>

template <typename T>
class ObserverPtr
{
public:
    ObserverPtr() : ptr_(nullptr)
    {
    }

    ObserverPtr(std::nullptr_t) : ptr_(nullptr)
    {
    }

    ObserverPtr(T* ptr) : ptr_(ptr)
    {
    }

    ObserverPtr(const std::unique_ptr<T>& uptr) : ptr_(uptr.get())
    {
    }

    ~ObserverPtr()
    {
        ptr_ = nullptr;
    }

    ObserverPtr(const ObserverPtr& other) noexcept : ptr_(other.ptr_)
    {
    }

    ObserverPtr& operator=(const ObserverPtr& other) noexcept
    {
        ptr_ = other.ptr_;
        return *this;
    }

    ObserverPtr(ObserverPtr&& other) noexcept
        : ptr_(std::exchange(other.ptr_, nullptr))
    {
    }

    ObserverPtr& operator=(ObserverPtr&& other) noexcept
    {
        if (this != &other)
        {
            ptr_ = std::exchange(other.ptr_, nullptr);
        }
        return *this;
    }

    T* operator->() const
    {
        assert(ptr_ != nullptr && "Dereferencing null ObserverPtr");
        return ptr_;
    }

    T& operator*() const
    {
        assert(ptr_ != nullptr && "Dereferencing null ObserverPtr");
        return *ptr_;
    }

    T* get() const { return ptr_; }

    explicit operator bool() const { return ptr_ != nullptr; }

    void reset(T* p = nullptr) { ptr_ = p; }

    friend bool operator==(const ObserverPtr& lhs, const ObserverPtr& rhs) noexcept
    {
        return lhs.ptr_ == rhs.ptr_;
    }

    friend bool operator!=(const ObserverPtr& lhs, const ObserverPtr& rhs) noexcept
    {
        return lhs.ptr_ != rhs.ptr_;
    }

    friend bool operator==(const ObserverPtr& lhs, std::nullptr_t) noexcept
    {
        return lhs.ptr_ == nullptr;
    }

    friend bool operator==(std::nullptr_t, const ObserverPtr& rhs) noexcept
    {
        return nullptr == rhs.ptr_;
    }

    friend bool operator!=(const ObserverPtr& lhs, std::nullptr_t) noexcept
    {
        return lhs.ptr_ != nullptr;
    }

    friend bool operator!=(std::nullptr_t, const ObserverPtr& rhs) noexcept
    {
        return nullptr != rhs.ptr_;
    }

    friend bool operator==(const ObserverPtr& lhs, T* rhs) noexcept
    {
        return lhs.ptr_ == rhs;
    }

    friend bool operator==(T* lhs, const ObserverPtr& rhs) noexcept
    {
        return lhs == rhs.ptr_;
    }

    friend bool operator!=(const ObserverPtr& lhs, T* rhs) noexcept
    {
        return lhs.ptr_ != rhs;
    }

    friend bool operator!=(T* lhs, const ObserverPtr& rhs) noexcept
    {
        return lhs != rhs.ptr_;
    }

private:
    T* ptr_;
};

template <typename T>
void swap(ObserverPtr<T>& lhs, ObserverPtr<T>& rhs) noexcept
{
    lhs.swap(rhs);
}
