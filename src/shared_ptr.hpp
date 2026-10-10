#ifndef SHARED_PTR_HPP
#define SHARED_PTR_HPP
#include <atomic>
#include <stdexcept>
#include <utility>

namespace ptrs::shared {

class ref_counter {
    template <typename T>
    friend class shared_ptr;
    std::atomic<int> count;

    ref_counter() : count(1) {
    }
};

template <typename T>
class shared_ptr {
    T *raw_ptr_ = nullptr;
    ref_counter *counter_;

    void release() {
        if (counter_ != nullptr) {
            if (--counter_->count == 0) {
                delete raw_ptr_;
                delete counter_;
            }
            counter_ = nullptr;
            raw_ptr_ = nullptr;
        }
    }

public:
    shared_ptr() : raw_ptr_(nullptr), counter_(nullptr) {
    }

    explicit shared_ptr(T *ptr) : raw_ptr_(ptr), counter_(new ref_counter()) {
    }

    shared_ptr(const shared_ptr &other)
        : raw_ptr_(other.raw_ptr_  // cppcheck-suppress copyCtorPointerCopying
          ),
          counter_(other.counter_  // cppcheck-suppress copyCtorPointerCopying
          ) {
        if (counter_ != nullptr) {
            counter_->count++;
        }
    }

    shared_ptr &operator=(const shared_ptr &other) {
        shared_ptr tmp(other);
        swap(*this, tmp);
        return *this;
    }

    shared_ptr(shared_ptr &&other) noexcept
        : raw_ptr_(std::exchange(other.raw_ptr_, nullptr)),
          counter_(std::exchange(other.counter_, nullptr)) {
    }

    shared_ptr &operator=(shared_ptr &&other) noexcept {
        shared_ptr tmp(std::move(other));
        swap(*this, tmp);
        return *this;
    }

    ~shared_ptr() {
        release();
    }

    void check_counter(const shared_ptr &other) const {
        if (other.counter_->count == 0) {
            delete other.counter_;
            delete other.raw_ptr_;
        }
    }

    T *get() const {
        return raw_ptr_;
    }

    explicit operator bool() const {
        return raw_ptr_ != nullptr;
    }

    friend void swap(shared_ptr &first, shared_ptr &second) noexcept {
        std::swap(first.raw_ptr_, second.raw_ptr_);
        std::swap(first.counter_, second.counter_);
    }

    T &operator*() const {
        return *raw_ptr_;
    }

    T *operator->() const {
        if (raw_ptr_ == nullptr) {
            throw std::runtime_error("Cannot dereference a null pointer");
        }
        return raw_ptr_;
    }

    bool operator==(const shared_ptr &other) const {
        return raw_ptr_ == other.raw_ptr_;
    }

    bool operator!=(const shared_ptr &other) const {
        return raw_ptr_ != other.raw_ptr_;
    }

    void reset(T *ptr = nullptr) {
        release();
        if (ptr != nullptr) {
            counter_ = new ref_counter();
            raw_ptr_ = ptr;
        }
    }
};

}  // namespace ptrs::shared

#endif
