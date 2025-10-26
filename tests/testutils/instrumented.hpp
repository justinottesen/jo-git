#pragma once

#include <atomic>
#include <type_traits>
#include <utility>

template <class T, class Tag = void> class Instrumented {
public:
    static inline std::atomic<unsigned int> default_ctor{0};
    static inline std::atomic<unsigned int> value_ctor{0};
    static inline std::atomic<unsigned int> copy_ctor{0};
    static inline std::atomic<unsigned int> move_ctor{0};
    static inline std::atomic<unsigned int> copy_assign{0};
    static inline std::atomic<unsigned int> move_assign{0};
    static inline std::atomic<unsigned int> dtor{0};

    static void reset_counts() {
        default_ctor = value_ctor = copy_ctor = move_ctor = copy_assign = move_assign = dtor = 0;
    }

    Instrumented() noexcept(std::is_nothrow_default_constructible_v<T>)
        : value_() {
        ++default_ctor;
    }

    template <class... Args>
    explicit Instrumented(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
        : value_(std::forward<Args>(args)...) {
        ++value_ctor;
    }

    Instrumented(const Instrumented& rhs) noexcept(std::is_nothrow_copy_constructible_v<T>)
        : value_(rhs.value_) {
        ++copy_ctor;
    }

    Instrumented(Instrumented&& rhs) noexcept(std::is_nothrow_move_constructible_v<T>)
        : value_(std::move(rhs.value_)) {
        ++move_ctor;
    }

    auto operator=(const Instrumented& rhs) noexcept(std::is_nothrow_copy_assignable_v<T>)
        -> Instrumented& {
        if (this != &rhs) { value_ = rhs.value_; }
        ++copy_assign;
        return *this;
    }
    auto operator=(Instrumented&& rhs) noexcept(std::is_nothrow_move_assignable_v<T>)
        -> Instrumented& {
        if (this != &rhs) { value_ = std::move(rhs.value_); }
        ++move_assign;
        return *this;
    }

    ~Instrumented() noexcept { ++dtor; }

    auto get() noexcept -> T& { return value_; }
    auto get() const noexcept -> const T& { return value_; }

    explicit operator T&() noexcept { return value_; }
    explicit operator const T&() const noexcept { return value_; }

private:
    T value_;
};

