// NOLINTBEGIN

#include <future>
#include <latch>
#include <stdexcept>

#include <common/singleton.hpp>
#include <gtest/gtest.h>

#include "../testutils/instrumented.hpp"

using jo::git::common::Singleton;

// A simple type to track
struct Foo {
    int x = 0;
    explicit Foo(int v) noexcept
        : x(v) {}

    Foo() noexcept = default;
};

TEST(Singleton, AutoConstrutInstance) {
    struct Tag {};
    using I = Instrumented<Foo, Tag>;

    struct Tracked : Singleton<I> {};

    I::reset_counts();

    EXPECT_FALSE(Tracked::initialized());

    auto& obj = Tracked::instance();

    EXPECT_TRUE(Tracked::initialized());

    EXPECT_EQ(I::default_ctor.load(), 1);
    EXPECT_EQ(I::value_ctor.load(), 0);
    EXPECT_EQ(I::copy_ctor.load(), 0);
    EXPECT_EQ(I::move_ctor.load(), 0);
    EXPECT_EQ(I::copy_assign.load(), 0);
    EXPECT_EQ(I::move_assign.load(), 0);
    EXPECT_EQ(I::dtor.load(), 0);

    EXPECT_EQ(obj.get().x, 0);
}

TEST(Singleton, InitDefaultConstructInstance) {
    struct Tag {};
    using I = Instrumented<Foo, Tag>;

    struct Tracked : Singleton<I, false> {};

    I::reset_counts();

    EXPECT_FALSE(Tracked::initialized());

    auto& obj = Tracked::init();

    EXPECT_TRUE(Tracked::initialized());

    EXPECT_EQ(I::default_ctor.load(), 1);
    EXPECT_EQ(I::value_ctor.load(), 0);
    EXPECT_EQ(I::copy_ctor.load(), 0);
    EXPECT_EQ(I::move_ctor.load(), 0);
    EXPECT_EQ(I::copy_assign.load(), 0);
    EXPECT_EQ(I::move_assign.load(), 0);
    EXPECT_EQ(I::dtor.load(), 0);

    EXPECT_EQ(obj.get().x, 0);
}

TEST(Singleton, InitValueConstructInstance) {
    struct Tag {};
    using I = Instrumented<Foo, Tag>;

    struct Tracked : Singleton<I, false> {};

    I::reset_counts();

    EXPECT_FALSE(Tracked::initialized());

    auto& obj = Tracked::init(7);

    EXPECT_TRUE(Tracked::initialized());

    EXPECT_EQ(I::default_ctor.load(), 0);
    EXPECT_EQ(I::value_ctor.load(), 1);
    EXPECT_EQ(I::copy_ctor.load(), 0);
    EXPECT_EQ(I::move_ctor.load(), 0);
    EXPECT_EQ(I::copy_assign.load(), 0);
    EXPECT_EQ(I::move_assign.load(), 0);
    EXPECT_EQ(I::dtor.load(), 0);

    EXPECT_EQ(obj.get().x, 7);
}

TEST(Singleton, NoCopiesRepeatedAccess) {
    struct Tag {};
    using I = Instrumented<Foo, Tag>;

    struct Tracked : Singleton<I, false> {};

    I::reset_counts();

    EXPECT_FALSE(Tracked::initialized());

    auto& obj = Tracked::init(11);

    EXPECT_TRUE(Tracked::initialized());

    auto& obj2 = Tracked::instance();
    auto& obj3 = Tracked::instance();

    EXPECT_EQ(I::default_ctor.load(), 0);
    EXPECT_EQ(I::value_ctor.load(), 1);
    EXPECT_EQ(I::copy_ctor.load(), 0);
    EXPECT_EQ(I::move_ctor.load(), 0);
    EXPECT_EQ(I::copy_assign.load(), 0);
    EXPECT_EQ(I::move_assign.load(), 0);
    EXPECT_EQ(I::dtor.load(), 0);

    EXPECT_EQ(&obj, &obj2);
    EXPECT_EQ(&obj2, &obj3);

    EXPECT_EQ(obj.get().x, 11);
}

TEST(Singleton, UninitThrows) {
    struct Tag {};
    using I = Instrumented<Foo, Tag>;

    struct Tracked : Singleton<I, false> {};

    I::reset_counts();

    EXPECT_THROW(Tracked::instance(), std::logic_error);

    EXPECT_FALSE(Tracked::initialized());

    EXPECT_EQ(I::default_ctor.load(), 0);
    EXPECT_EQ(I::value_ctor.load(), 0);
    EXPECT_EQ(I::copy_ctor.load(), 0);
    EXPECT_EQ(I::move_ctor.load(), 0);
    EXPECT_EQ(I::copy_assign.load(), 0);
    EXPECT_EQ(I::move_assign.load(), 0);
    EXPECT_EQ(I::dtor.load(), 0);
}

TEST(Singleton, MultiInitThrows) {
    struct Tag {};
    using I = Instrumented<Foo, Tag>;

    struct Tracked : Singleton<I, false> {};

    I::reset_counts();

    EXPECT_FALSE(Tracked::initialized());

    Tracked::init(10);
    EXPECT_THROW(Tracked::init(11), std::logic_error);

    EXPECT_TRUE(Tracked::initialized());

    EXPECT_EQ(I::default_ctor.load(), 0);
    EXPECT_EQ(I::value_ctor.load(), 1);
    EXPECT_EQ(I::copy_ctor.load(), 0);
    EXPECT_EQ(I::move_ctor.load(), 0);
    EXPECT_EQ(I::copy_assign.load(), 0);
    EXPECT_EQ(I::move_assign.load(), 0);
    EXPECT_EQ(I::dtor.load(), 0);

    EXPECT_EQ(Tracked::instance().get().x, 10);
}

TEST(Singleton, AutoInitManyThreadsOneConstruction) {
    struct Tag {};
    using I = Instrumented<Foo, Tag>;
    struct Tracked : Singleton<I> {};

    I::reset_counts();

    constexpr int            N = 64;
    std::latch               go(N);
    std::vector<std::thread> ths;
    ths.reserve(N);
    std::vector<I*> got(N, nullptr);

    for (int i = 0; i < N; ++i) {
        ths.emplace_back([i, &go, &got] {
            go.arrive_and_wait();
            got[i] = &Tracked::instance();
        });
    }
    for (auto& t : ths) t.join();

    for (int i = 1; i < N; ++i) { EXPECT_EQ(got[0], got[i]); }
    EXPECT_EQ(I::default_ctor.load(), 1);
    EXPECT_EQ(I::value_ctor.load(), 0);
    EXPECT_EQ(I::copy_ctor.load(), 0);
    EXPECT_EQ(I::move_ctor.load(), 0);
    EXPECT_EQ(I::copy_assign.load(), 0);
    EXPECT_EQ(I::move_assign.load(), 0);
    EXPECT_EQ(I::dtor.load(), 0);
}

// Blocking payload for this test
struct BlockingPayload {
    static inline std::shared_future<void> gate;
    static void set_gate(std::shared_future<void> g) { gate = std::move(g); }
    BlockingPayload() noexcept { gate.wait(); }    // block until released
    int v = 123;
};

TEST(Singleton, WaitersBlockDuringInitializingThenProceed) {
    struct Tag {};
    using I = Instrumented<BlockingPayload, Tag>;
    struct Tracked : Singleton<I, /*DefaultOnInstance=*/true> {};

    I::reset_counts();

    std::promise<void> pr;
    BlockingPayload::set_gate(pr.get_future().share());

    // Thread that triggers initialization and blocks in ctor
    std::thread init_thread([] {
        (void)Tracked::instance();    // enters Initializing and waits in ctor
    });

    // Give init thread a moment to enter ctor (not strictly necessary)
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Now start waiters; they should wait, then return the same instance
    constexpr int            N = 16;
    std::vector<std::thread> waiters;
    waiters.reserve(N);
    std::vector<I*> got(N, nullptr);
    for (int i = 0; i < N; ++i) {
        waiters.emplace_back([i, &got] { got[i] = &Tracked::instance(); });
    }

    // Release ctor
    pr.set_value();

    init_thread.join();
    for (auto& t : waiters) t.join();

    // All pointers identical
    for (int i = 1; i < N; ++i) { EXPECT_EQ(got[0], got[i]); }
    EXPECT_TRUE(Tracked::initialized());
    EXPECT_EQ(I::default_ctor.load(), 1);
    EXPECT_EQ(I::value_ctor.load(), 0);
}

TEST(Singleton, RaceInitArgsVsInstanceAutoInitWinnerSetsValue) {
    struct Tag {};
    using I = Instrumented<Foo, Tag>;
    struct Tracked : Singleton<I, /*DefaultOnInstance=*/true> {};

    I::reset_counts();

    std::thread t_init([] { (void)Tracked::init(7); });

    constexpr int            N = 16;
    std::vector<std::thread> ths;
    ths.reserve(N);
    for (int i = 0; i < N; ++i) {
        ths.emplace_back([] {
            // Should not throw; either waits or sees Initialized
            (void)Tracked::instance();
        });
    }

    t_init.join();
    for (auto& t : ths) t.join();

    // Value construction must be exactly once; no default construction
    EXPECT_EQ(I::value_ctor.load(), 1);
    EXPECT_EQ(I::default_ctor.load(), 0);
    EXPECT_EQ(Tracked::instance().get().x, 7);
}

TEST(Singleton, RaceDoubleInitOneSucceedsOtherThrows) {
    struct Tag {};
    using I = Instrumented<Foo, Tag>;
    struct Tracked : Singleton<I, /*DefaultOnInstance=*/false> {};

    I::reset_counts();

    std::atomic<int> throws{0};

    std::thread a([&] {
        try {
            (void)Tracked::init(1);
        } catch (const std::logic_error&) { ++throws; }
    });
    std::thread b([&] {
        try {
            (void)Tracked::init(2);
        } catch (const std::logic_error&) { ++throws; }
    });

    a.join();
    b.join();

    EXPECT_EQ(throws.load(), 1);           // exactly one throws
    EXPECT_EQ(I::value_ctor.load(), 1);    // only one construction
    int v = Tracked::instance().get().x;
    EXPECT_TRUE(v == 1 || v == 2);         // winner’s value
}

// Blocking version of Foo so we can hold Initializing state
struct BlockingFoo {
    static inline std::shared_future<void> gate;
    static void set_gate(std::shared_future<void> g) { gate = std::move(g); }
    explicit BlockingFoo(int v) noexcept
        : x(v) {
        gate.wait();
    }
    int x;
};

TEST(Singleton, InstanceWaitsWhileInitInProgressDefaultOnInstanceFalse) {
    struct Tag {};
    using I = Instrumented<BlockingFoo, Tag>;
    struct Tracked : Singleton<I, /*DefaultOnInstance=*/false> {};

    I::reset_counts();

    std::promise<void> pr;
    BlockingFoo::set_gate(pr.get_future().share());

    // Start init in a thread; it will block in the ctor
    std::thread t_init([] { (void)Tracked::init(5); });

    // Concurrent callers of instance() should wait and then return the same instance
    constexpr int            N = 16;
    std::vector<std::thread> ths;
    ths.reserve(N);
    std::vector<I*> got(N, nullptr);
    for (int i = 0; i < N; ++i) {
        ths.emplace_back([i, &got] { got[i] = &Tracked::instance(); });
    }

    // Release ctor
    pr.set_value();

    t_init.join();
    for (auto& t : ths) t.join();

    for (int i = 1; i < N; ++i) { EXPECT_EQ(got[0], got[i]); }
    EXPECT_EQ(I::value_ctor.load(), 1);
    EXPECT_EQ(Tracked::instance().get().x, 5);
}

TEST(Singleton, InitAfterInitThrowsAndKeepsOriginal) {
    struct Tag {};
    using I = Instrumented<Foo, Tag>;
    struct Tracked : Singleton<I, /*DefaultOnInstance=*/false> {};

    I::reset_counts();

    auto& first = Tracked::init(3);
    EXPECT_TRUE(Tracked::initialized());
    EXPECT_THROW((void)Tracked::init(4), std::logic_error);

    // Original instance unchanged
    EXPECT_EQ(Tracked::instance().get().x, 3);
    EXPECT_EQ(&Tracked::instance(), &first);

    EXPECT_EQ(I::value_ctor.load(), 1);
    EXPECT_EQ(I::default_ctor.load(), 0);
    EXPECT_EQ(I::copy_ctor.load(), 0);
    EXPECT_EQ(I::move_ctor.load(), 0);
}

TEST(Singleton, InitAfterAutoInitThrowsAndKeepsAutoInstance) {
    struct Tag {};
    using I = Instrumented<Foo, Tag>;
    struct Tracked : Singleton<I, /*DefaultOnInstance=*/true> {};

    I::reset_counts();

    auto& autoObj = Tracked::instance();    // default-constructs
    EXPECT_TRUE(Tracked::initialized());
    EXPECT_THROW((void)Tracked::init(9), std::logic_error);

    EXPECT_EQ(&Tracked::instance(), &autoObj);
    EXPECT_EQ(Tracked::instance().get().x, 0);

    EXPECT_EQ(I::default_ctor.load(), 1);
    EXPECT_EQ(I::value_ctor.load(), 0);
}

TEST(Singleton, InitializedFlagTransitionsAndSticks) {
    struct Tag {};
    using I = Instrumented<Foo, Tag>;
    struct Tracked : Singleton<I, /*DefaultOnInstance=*/false> {};

    I::reset_counts();

    EXPECT_FALSE(Tracked::initialized());
    (void)Tracked::init(42);
    EXPECT_TRUE(Tracked::initialized());
    (void)Tracked::instance();    // still true
    EXPECT_TRUE(Tracked::initialized());

    EXPECT_EQ(I::value_ctor.load(), 1);
    EXPECT_EQ(Tracked::instance().get().x, 42);
}

TEST(Singleton, DifferentTypesAreIndependent) {
    struct TagA {};
    struct TagB {};
    using IA = Instrumented<Foo, TagA>;
    using IB = Instrumented<Foo, TagB>;

    struct A : Singleton<IA, false> {};
    struct B : Singleton<IB, true> {};

    IA::reset_counts();
    IB::reset_counts();

    auto& a = A::init(5);       // explicit init
    auto& b = B::instance();    // auto-init default

    EXPECT_EQ(a.get().x, 5);
    EXPECT_EQ(b.get().x, 0);

    // A’s counters reflect only A
    EXPECT_EQ(IA::value_ctor.load(), 1);
    EXPECT_EQ(IA::default_ctor.load(), 0);

    // B’s counters reflect only B
    EXPECT_EQ(IB::default_ctor.load(), 1);
    EXPECT_EQ(IB::value_ctor.load(), 0);
}

TEST(Singleton, ExplicitInitAllowedWhenAutoInitEnabledConstructsOnce) {
    struct Tag {};
    using I = Instrumented<Foo, Tag>;
    struct Tracked : Singleton<I, /*DefaultOnInstance=*/true> {};

    I::reset_counts();

    auto& a = Tracked::init();        // explicit default construction
    auto& b = Tracked::instance();    // should be same object

    EXPECT_EQ(&a, &b);
    EXPECT_EQ(I::default_ctor.load(), 1);
    EXPECT_EQ(I::value_ctor.load(), 0);
}

// NOLINTEND
