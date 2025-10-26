#pragma once

#include <atomic>
#include <cassert>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <utility>

#include <common/macros.hpp>

namespace jo::git::common {

template <typename T, bool DefaultOnInstance = true>
    requires(!DefaultOnInstance || std::is_nothrow_constructible_v<T>)
class Singleton {
public:
    DELETE_COPY_MOVE(Singleton);

    template <class... Args>
        requires std::is_nothrow_constructible_v<T, Args...>
    static auto init(Args&&... args) -> T& {
        if (!trySetInitializing()) {
            throw std::logic_error("Singleton::init called more than once");
        }
        return init_UNSAFE(std::forward<Args>(args)...);
    }

    static auto instance() -> T& {
        // Check the state of the object
        switch (m_state.load(std::memory_order_acquire)) {
            case State::Uninitialized:
                // If uninitialized, try to set it initialized
                if constexpr (!DefaultOnInstance) {
                    throw std::logic_error("Singleton not initialized, call Singleton::init(...)");
                } else {
                    if (trySetInitializing()) { return init_UNSAFE(); }
                    // If we fail the above, someone else set it to initializing before us
                }
                [[fallthrough]];
            case State::Initializing:
                // If it is initializing, someone else is doing the work, we just spin and wait
                m_state.wait(State::Initializing, std::memory_order_acquire);
                [[fallthrough]];
            case State::Initialized:
                // If it is initialized, we just return the reference
                return ref();
        }
        std::unreachable();
    }

    static auto initialized() -> bool {
        return m_state.load(std::memory_order_acquire) == State::Initialized;
    }

protected:
    DEFAULT_CTOR_DTOR(Singleton);

private:
    static auto storage() noexcept -> std::optional<T>& {
        static std::optional<T> storage;
        return storage;
    }

    static auto addr() noexcept -> T* {
        // NOLINTNEXTLINE
        return &*storage();
    }

    static auto ref() noexcept -> T& { return *storage(); }

    enum class State : uint8_t { Uninitialized, Initializing, Initialized };

    static inline std::atomic<State> m_state{State::Uninitialized};

    static auto trySetInitializing() -> bool {
        State state = State::Uninitialized;
        return m_state.compare_exchange_strong(state, State::Initializing,
                                               std::memory_order_acq_rel);
    }

    template <class... Args> static auto init_UNSAFE(Args&&... args) -> T& {
        storage().emplace(std::forward<Args>(args)...);
        m_state.store(State::Initialized, std::memory_order_release);
        m_state.notify_all();
        return ref();
    }
};

}    // namespace jo::git::common
