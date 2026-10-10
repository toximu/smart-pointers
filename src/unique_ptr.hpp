#ifndef UNIQUE_PTR_HPP
#define UNIQUE_PTR_HPP
#include <functional>
#include <stdexcept>
#include <utility>

namespace ptrs::unique {

template <typename T>
class DefaultDelete {
public:
    void operator()(T *ptr) const {
        delete ptr;
    }
};

template <typename T, typename Deleter = DefaultDelete<T>>
class unique_ptr {
    T *raw_ptr_;
    [[no_unique_address]] Deleter deleter_;

public:
    ~unique_ptr() {
        if (raw_ptr_) {
            deleter_(raw_ptr_);
        }
    }

    T &operator*() const {
        if (raw_ptr_ == nullptr) {
            throw std::runtime_error("Cannot dereference a null pointer");
        }
        return *raw_ptr_;
    }

    T *get() const noexcept {
        return raw_ptr_;
    }

    T *operator->() const {
        if (raw_ptr_ == nullptr) {
            throw std::runtime_error("Cannot dereference a null pointer");
        }
        return raw_ptr_;
    }

    explicit operator bool() const noexcept {
        return raw_ptr_ != nullptr;
    }

    T *release() {
        return std::exchange(raw_ptr_, nullptr);
    }

    bool operator==(const unique_ptr &other) const noexcept {
        return raw_ptr_ == other.raw_ptr_;
    }

    bool operator!=(const unique_ptr &other) const noexcept {
        return raw_ptr_ != other.raw_ptr_;
    }

    void reset(T *ptr = nullptr) noexcept {
        if (raw_ptr_) {
            deleter_(raw_ptr_);
        }
        raw_ptr_ = ptr;
    }

    unique_ptr() : raw_ptr_(nullptr), deleter_(Deleter()) {
    }

    explicit unique_ptr(T *ptr) : raw_ptr_(ptr), deleter_(Deleter()) {
    }

    explicit unique_ptr(T *ptr, const Deleter &deleter)
        : raw_ptr_(ptr), deleter_(deleter) {
    }

    explicit unique_ptr(T *ptr, Deleter &&deleter)
        : raw_ptr_(ptr), deleter_(std::move(deleter)) {
    }

    explicit unique_ptr(std::nullptr_t) : raw_ptr_(nullptr) {
    }

    unique_ptr(const unique_ptr &) = delete;
    void operator=(const unique_ptr &) = delete;

    unique_ptr(unique_ptr &&other) noexcept
        : raw_ptr_(std::exchange(other.raw_ptr_, nullptr)),
          deleter_(std::move(other.deleter_)) {
    }

    unique_ptr &operator=(unique_ptr &&other) noexcept {
        if (this != &other) {
            reset(std::exchange(other.raw_ptr_, nullptr));
            deleter_ = std::move(other.deleter_);
        }
        return *this;
    }

    friend void swap(unique_ptr &first, unique_ptr &second) noexcept {
        std::swap(first.raw_ptr_, second.raw_ptr_);
        std::swap(first.deleter_, second.deleter_);
    }
};

}  // namespace ptrs::unique

#endif
