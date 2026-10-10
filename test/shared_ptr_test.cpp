#ifdef TEST_STD_PTRS
#include <memory>
#else
#include "shared_ptr.hpp"
// NOLINTNEXTLINE(readability-duplicate-include)
#include "shared_ptr.hpp"  // Ensure include guards are present.
#endif
#include <iostream>  // https://github.com/onqtam/doctest/issues/356
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>
#include "doctest.h"
#include "test_utils.hpp"

// NOLINTBEGIN(readability-function-cognitive-complexity)
// NOLINTBEGIN(misc-use-anonymous-namespace)

#ifdef TEST_SHARED_PTR
namespace {

#ifdef TEST_STD_PTRS
using std::shared_ptr;
#else
using ptrs::shared::shared_ptr;
#endif

#ifdef TEST_STD_PTRS
TEST_CASE("std::shared_ptr<int> default constructor")
#else
TEST_CASE("ptrs::shared::shared_ptr<int> default constructor")
#endif
{
    const shared_ptr<int> p;
    CHECK(p.get() == nullptr);
    CHECK(!p);
}

TEST_CASE("shared_ptr incomplete type operations") {
    // No runtime checks, just ensure compilation.
    [[maybe_unused]] auto not_invoked = []() {
        struct IncompleteType;
        shared_ptr<IncompleteType> &p1 = *new shared_ptr<IncompleteType>();
        shared_ptr<IncompleteType> &p2 =
            *new shared_ptr<IncompleteType>(nullptr);
        shared_ptr<IncompleteType> &p3 = *new shared_ptr<IncompleteType>(
            static_cast<IncompleteType *>(nullptr)
        );
        [[maybe_unused]] const shared_ptr<IncompleteType> &p4 =
            *new shared_ptr<IncompleteType>(std::move(p3));
        [[maybe_unused]] const shared_ptr<IncompleteType> &p5 =
            *new shared_ptr<IncompleteType>(p4);
        swap(p1, p2);
        static_cast<void>(static_cast<bool>(p1));
        static_cast<void>(p1.get());
        static_cast<void>(*p1);
        static_cast<void>(p1 == p2);
        static_cast<void>(p1 != p2);
    };
}

TEST_CASE("shared_ptr default constructor is implicit") {
    [[maybe_unused]] const shared_ptr<int> p = {};
}

TEST_CASE("shared_ptr nullptr") {
    const shared_ptr<int> p(nullptr);
    CHECK(p.get() == nullptr);
    CHECK(!p);
}

TEST_CASE("shared_ptr new-delete") {
    const shared_ptr<int> p(new int(10));
    CHECK(p);
}

TEST_CASE("shared_ptr constructor is explicit") {
    CHECK(!std::is_convertible_v<int *, shared_ptr<int>>);
}

TEST_CASE("shared_ptr non-empty getters") {
    struct Foo {
        int value = 10;
    };

    CHECK(!std::is_convertible_v<shared_ptr<Foo>, bool>);
    Foo *raw_p = new Foo();
    const shared_ptr<Foo> p(raw_p);
    // REQUIRE() stops the test in case of failure.
    REQUIRE(p.get() == raw_p);
    REQUIRE(p);
    REQUIRE(static_cast<bool>(p));

    CHECK((*p).value == 10);
    CHECK(&*p == raw_p);
    CHECK(p->value == 10);

    (*p).value = 20;
    CHECK((*p).value == 20);
    CHECK(&*p == raw_p);
    CHECK(p->value == 20);

    p->value = 30;
    CHECK((*p).value == 30);
    CHECK(&*p == raw_p);
    CHECK(p->value == 30);

    p.get()->value = 40;  // NOLINT(readability-redundant-smartptr-get)
    CHECK((*p).value == 40);
    CHECK(&*p == raw_p);
    CHECK(p->value == 40);
}

#if defined(__GNUC__) && !defined(__clang__)
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=108565
#pragma GCC diagnostic ignored "-Wuse-after-free"
#endif

void check_after_copy(
    int *const raw_p,
    const shared_ptr<int> &p1,
    const shared_ptr<int> &p2
) {
    REQUIRE(p2.get() == raw_p);
    if (raw_p != nullptr) {
        REQUIRE(p2);
        CHECK(&*p2 == raw_p);
    } else {
        CHECK(!p2);
    }

    REQUIRE(p1.get() == raw_p);
    if (raw_p != nullptr) {
        REQUIRE(p1);
        CHECK(&*p1 == raw_p);
    } else {
        CHECK(!p1);
    }
}

TEST_CASE("shared_ptr copy constructor") {
    auto test = [](int arg_non_empty) {
        int *const raw_p = arg_non_empty == 1 ? new int(10) : nullptr;
        const shared_ptr<int> p1(raw_p);
        // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
        const shared_ptr<int> p2 = p1;
        check_after_copy(raw_p, p1, p2);
    };
    SUBCASE("arg_non_empty is 0") {
        test(0);
    }
    SUBCASE("arg_non_empty is 1") {
        test(1);
    }
}

TEST_CASE("shared_ptr copy assignment") {
    auto test = [](int arg_non_empty) {
        int *const raw_p = arg_non_empty == 1 ? new int(10) : nullptr;
        const shared_ptr<int> p1(raw_p);

        SUBCASE("over empty") {
            shared_ptr<int> p2;
            p2 = p1;
            check_after_copy(raw_p, p1, p2);
        }

        SUBCASE("over non-empty unique owner") {
            shared_ptr<int> p2(new int(20));
            p2 = p1;
            check_after_copy(raw_p, p1, p2);
        }

        SUBCASE("over non-empty shared owner") {
            shared_ptr<int> p2(new int(20));
            const shared_ptr<int> p2dup(p2);
            p2 = p1;
            check_after_copy(raw_p, p1, p2);
            CHECK(*p2dup == 20);
        }
    };
    SUBCASE("arg_non_empty is 0") {
        test(0);
    }
    SUBCASE("arg_non_empty is 1") {
        test(1);
    }
}

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-assign-overloaded"
#endif
TEST_CASE("shared_ptr copy self-assignment of empty") {
    shared_ptr<int> p;
    p = p;
    CHECK(p.get() == nullptr);
    CHECK(!p);
}

TEST_CASE("shared_ptr copy self-assignment of non-empty") {
    int *raw_p = new int(10);
    shared_ptr<int> p(raw_p);
    p = p;
    REQUIRE(p.get() == raw_p);
    REQUIRE(p);
    CHECK(&*p == raw_p);
}
#ifdef __clang__
#pragma clang diagnostic pop
#endif

void check_after_move(
    int *const raw_p,
    const shared_ptr<int> &p1,
    const shared_ptr<int> &p2
) {
    REQUIRE(p2.get() == raw_p);
    if (raw_p != nullptr) {
        REQUIRE(p2);
        CHECK(&*p2 == raw_p);
    } else {
        CHECK(!p2);
    }

    CHECK(p1.get() == nullptr);
    CHECK(!!p1 == false);
}

TEST_CASE("shared_ptr move constructor") {
    auto test = [](int arg_non_empty) {
        int *const raw_p = arg_non_empty == 1 ? new int(10) : nullptr;
        shared_ptr<int> p1(raw_p);
        const shared_ptr<int> p2 = std::move(p1);
        check_after_move(raw_p, p1, p2);  // NOLINT(bugprone-use-after-move)
    };
    SUBCASE("arg_non_empty is 0") {
        test(0);
    }
    SUBCASE("arg_non_empty is 1") {
        test(1);
    }
}

TEST_CASE("shared_ptr move assignment") {
    auto test = [](int arg_non_empty) {
        int *const raw_p = arg_non_empty == 1 ? new int(10) : nullptr;
        shared_ptr<int> p1(raw_p);

        SUBCASE("over empty") {
            shared_ptr<int> p2;
            p2 = std::move(p1);
            // NOLINTNEXTLINE(bugprone-use-after-move)
            check_after_move(raw_p, p1, p2);
        }

        SUBCASE("over non-empty unique owner") {
            shared_ptr<int> p2(new int(20));
            p2 = std::move(p1);
            // NOLINTNEXTLINE(bugprone-use-after-move)
            check_after_move(raw_p, p1, p2);
        }

        SUBCASE("over non-empty shared owner") {
            shared_ptr<int> p2(new int(20));
            const shared_ptr<int> p2dup(p2);
            p2 = std::move(p1);
            // NOLINTNEXTLINE(bugprone-use-after-move)
            check_after_move(raw_p, p1, p2);
            CHECK(*p2dup == 20);
        }
    };
    SUBCASE("arg_non_empty is 0") {
        test(0);
    }
    SUBCASE("arg_non_empty is 1") {
        test(1);
    }
}

// See
// https://stackoverflow.com/questions/13127455/what-does-the-standard-library-guarantee-about-self-move-assignment
// and
// https://stackoverflow.com/questions/13129031/on-implementing-stdswap-in-terms-of-move-assignment-and-move-constructor
// I believe that requiring move self-assignment to be a no-op is a safer bet.
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-move"
#endif
TEST_CASE("shared_ptr move self-assignment of empty") {
    shared_ptr<int> p;
    p = std::move(p);
    CHECK(p.get() == nullptr);  // NOLINT(bugprone-use-after-move)
    CHECK(!p);
}

TEST_CASE("shared_ptr move self-assignment of non-empty") {
    int *raw_p = new int(10);
    shared_ptr<int> p(raw_p);
    p = std::move(p);
    REQUIRE(p.get() == raw_p);  // NOLINT(bugprone-use-after-move)
    REQUIRE(p);
    CHECK(&*p == raw_p);
}
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
#ifdef __clang__
#pragma clang diagnostic pop
#endif

TEST_CASE("shared_ptr reset") {
    const auto test = [](shared_ptr<int> &p) {
        int *raw_p = nullptr;
        SUBCASE("no arguments") {
            p.reset();
        }
        SUBCASE("to nullptr") {
            p.reset(raw_p);
        }
        SUBCASE("to a new pointer") {
            raw_p = new int(20);
            p.reset(raw_p);
        }

        REQUIRE(p.get() == raw_p);
        if (raw_p != nullptr) {
            REQUIRE(p);
            CHECK(&*p == raw_p);
        } else {
            REQUIRE(!p);
        }
    };

    SUBCASE("originally empty") {
        shared_ptr<int> p;
        test(p);
    }

    SUBCASE("originally unique owner") {
        shared_ptr<int> p(new int(10));
        test(p);
    }

    SUBCASE("originally shared owner") {
        shared_ptr<int> p(new int(10));
        const shared_ptr<int> p2(p);
        test(p);
        CHECK(*p2 == 10);
    }
}

TEST_CASE("shared_ptr swap") {
    int *raw_p1 = new int(10);
    int *raw_p2 = new int(20);
    shared_ptr<int> p1(raw_p1);
    shared_ptr<int> p2(raw_p2);
    swap(p1, p2);

    CHECK(p1);
    CHECK(p2);
    CHECK(p1.get() == raw_p2);
    CHECK(p2.get() == raw_p1);
}

TEST_CASE("shared_ptr operator== and operator!=") {
    const shared_ptr<int> null1;
    const shared_ptr<int> null2;
    const shared_ptr<int> p1a(new int(10));
    // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
    const shared_ptr<int> p1b(p1a);
    const shared_ptr<int> p2(new int(10));

    CHECK(null1 == null1);

    CHECK(null1 == null2);
    CHECK(null1 != p1a);
    CHECK(null1 != p1b);
    CHECK(null1 != p2);

    CHECK(null2 == null1);
    CHECK(p1a != null1);
    CHECK(p1b != null1);
    CHECK(p2 != null1);

    CHECK(p1a == p1a);
    CHECK(p1a == p1b);
    CHECK(p1a != p2);

    CHECK(p1b == p1a);
    CHECK(p1b == p1b);
    CHECK(p1b != p2);

    CHECK(p2 != p1a);
    CHECK(p2 != p1b);
    CHECK(p2 == p2);
}

}  // namespace

#ifdef TEST_SHARED_PTR_CONST_THREAD_SAFE
// You may need to use `-pthread` on Linux/macOS to compile these tests.
// With `CMakeLists.txt`, use this:
//     find_package(Threads)
//     target_link_libraries(your-executable-name ${CMAKE_THREAD_LIBS_INIT})

TEST_CASE(
    "shared_ptr can be copy-constructed thread-safely from distinct instances"
) {
    struct Foo {
        std::string s = std::string(1'000'000, 'x');
    };

    const shared_ptr<Foo> p_orig(new Foo());

    constexpr int THREADS = 10;
#ifdef EXPECT_VALGRIND
    constexpr int OPERATIONS = 100'000;
#else
    constexpr int OPERATIONS = 1'000'000;
#endif

    std::vector<shared_ptr<Foo>> ps(THREADS, p_orig);
    std::vector<std::thread> threads;
    threads.reserve(THREADS);
    for (int thread_id = 0; thread_id < THREADS; ++thread_id) {
        threads.emplace_back([&, thread_id]() {
            for (int i = 0; i < OPERATIONS; ++i) {
                const shared_ptr<Foo> p(ps[thread_id]);
                static_cast<void>(*p);
            }
        });
    }
    for (auto &t : threads) {
        t.join();
    }
}
#endif  // TEST_SHARED_PTR_CONST_THREAD_SAFE
#endif  // TEST_SHARED_PTR

// NOLINTEND(misc-use-anonymous-namespace)
// NOLINTEND(readability-function-cognitive-complexity)
