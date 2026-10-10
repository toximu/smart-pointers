#ifdef TEST_STD_PTRS
#include <memory>
#else
#include "unique_ptr.hpp"
// NOLINTNEXTLINE(readability-duplicate-include)
#include "unique_ptr.hpp"  // Ensure include guards are present.
#endif

#include <iostream>  // https://github.com/onqtam/doctest/issues/356
#include <type_traits>
#include <utility>
#include "doctest.h"
#include "test_utils.hpp"

// NOLINTBEGIN(readability-function-cognitive-complexity)
// NOLINTBEGIN(misc-use-anonymous-namespace)

#ifdef TEST_UNIQUE_PTR

namespace {
#ifdef TEST_STD_PTRS
using std::unique_ptr;
#else
using ptrs::unique::unique_ptr;
#endif

#ifdef TEST_STD_PTRS
TEST_CASE("std::unique_ptr<int> default constructor")
#else
TEST_CASE("ptrs::unique::unique_ptr<int> default constructor")
#endif
{
    const unique_ptr<int> p;
    CHECK(p.get() == nullptr);
    CHECK(!p);
}

TEST_CASE("unique_ptr incomplete type operations") {
    // No runtime checks, just ensure compilation.
    [[maybe_unused]] auto not_invoked = []() {
        struct IncompleteType;
        unique_ptr<IncompleteType> &p1 = *new unique_ptr<IncompleteType>();
        unique_ptr<IncompleteType> &p2 =
            *new unique_ptr<IncompleteType>(nullptr);
        unique_ptr<IncompleteType> &p3 = *new unique_ptr<IncompleteType>(
            static_cast<IncompleteType *>(nullptr)
        );
        [[maybe_unused]] const unique_ptr<IncompleteType> &p4 =
            *new unique_ptr<IncompleteType>(std::move(p3));
        swap(p1, p2);
        static_cast<void>(static_cast<bool>(p1));
        static_cast<void>(p1.get());
        static_cast<void>(p1.release());
        static_cast<void>(*p1);
        static_cast<void>(p1 == p2);
        static_cast<void>(p1 != p2);
    };
}

TEST_CASE("unique_ptr default constructor is implicit") {
    const unique_ptr<int> p = {};
}

TEST_CASE("unique_ptr nullptr") {
    const unique_ptr<int> p(nullptr);
    CHECK(p.get() == nullptr);
    CHECK(!p);
}

TEST_CASE("unique_ptr new-delete") {
    const unique_ptr<int> p(new int(10));
    CHECK(p);
}

TEST_CASE("unique_ptr constructor is explicit") {
    CHECK(!std::is_convertible_v<int *, unique_ptr<int>>);
}

#ifdef TEST_UNIQUE_PTR_DELETER
TEST_CASE("unique_ptr custom copyable deleter") {
    struct Deleter {
        int *counter;  // NOLINT(misc-non-private-member-variables-in-classes)

        // NOLINTNEXTLINE(readability-make-member-function-const)
        void operator()(int *) {
            ++*counter;
        }
    };

    int data = 10;
    int counter = 0;
    const Deleter d{&counter};
    CHECK(counter == 0);
    {
        const unique_ptr<int, Deleter> p(&data, d);
        CHECK(counter == 0);
    }
    CHECK(counter == 1);
}

TEST_CASE("unique_ptr custom move-only non-default-constructible deleter") {
    struct Deleter {
        int *counter;  // NOLINT(misc-non-private-member-variables-in-classes)

        explicit Deleter(int *counter_) : counter(counter_) {
        }

        Deleter(const Deleter &) = delete;
        Deleter &operator=(const Deleter &) = delete;
        Deleter(Deleter &&) = default;
        Deleter &operator=(Deleter &&) = default;
        ~Deleter() = default;

        // NOLINTNEXTLINE(readability-make-member-function-const)
        void operator()(const int *x) {
            if (x != nullptr) {
                ++*counter;
            }
        }
    };

    int data = 10;
    int counter = 0;
    CHECK(counter == 0);
    {
        Deleter d{&counter};
        const unique_ptr<int, Deleter> p(&data, std::move(d));
        CHECK(counter == 0);
    }
    CHECK(counter == 1);
    {
        Deleter d1{&counter};
        unique_ptr<int, Deleter> p1(&data, std::move(d1));
        unique_ptr<int, Deleter> p2 = std::move(p1);
        CHECK(counter == 1);

        Deleter d3{&counter};
        [[maybe_unused]] unique_ptr<int, Deleter> p3(&data, std::move(d3));
        CHECK(counter == 1);

        p3 = std::move(p2);
        CHECK(counter == 2);
    }
    CHECK(counter == 3);
}
#endif  // TEST_UNIQUE_PTR_DELETER

TEST_CASE("unique_ptr non-copyable") {
    CHECK(!std::is_copy_constructible_v<unique_ptr<int>>);
    CHECK(!std::is_copy_assignable_v<unique_ptr<int>>);
}

TEST_CASE("unique_ptr non-array getters") {
    struct Foo {
        int value = 10;
    };

    CHECK(!std::is_convertible_v<unique_ptr<Foo>, bool>);
    SUBCASE("empty") {
        const unique_ptr<Foo> p;
        CHECK(p.get() == nullptr);
        CHECK(!p);
    }
    SUBCASE("non-empty non-array") {
        Foo *raw_p = new Foo();
        const unique_ptr<Foo> p(raw_p);
        // REQUIRE() stops the test in case of failure.
        REQUIRE(p.get() == raw_p);
        REQUIRE(p);

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
}

TEST_CASE("unique_ptr move constructor") {
    int *raw_p = new int(10);
    unique_ptr<int> p1(raw_p);
    const unique_ptr<int> p2 = std::move(p1);

    CHECK(p1.get() == nullptr);  // NOLINT(bugprone-use-after-move)
    // cppcheck-suppress accessMoved
    CHECK(!p1);

    REQUIRE(p2.get() == raw_p);
    REQUIRE(p2);
    CHECK(&*p2 == raw_p);
}

TEST_CASE("unique_ptr move assignment over empty") {
    int *raw_p = new int(10);
    unique_ptr<int> p1(raw_p);
    unique_ptr<int> p2;

    p2 = std::move(p1);

    CHECK(p1.get() == nullptr);  // NOLINT(bugprone-use-after-move)
    // cppcheck-suppress accessMoved
    CHECK(!p1);

    REQUIRE(p2.get() == raw_p);
    REQUIRE(p2);
    CHECK(&*p2 == raw_p);
}

TEST_CASE("unique_ptr move assignment over non-empty") {
    int *raw_p = new int(10);
    unique_ptr<int> p1(raw_p);
    unique_ptr<int> p2(new int(20));

    p2 = std::move(p1);

    CHECK(p1.get() == nullptr);  // NOLINT(bugprone-use-after-move)
    // cppcheck-suppress accessMoved
    CHECK(!p1);

    REQUIRE(p2.get() == raw_p);
    REQUIRE(p2);
    CHECK(&*p2 == raw_p);
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
TEST_CASE("unique_ptr move self-assignment of empty") {
    unique_ptr<int> p;
    p = std::move(p);
    CHECK(p.get() == nullptr);  // NOLINT(bugprone-use-after-move)
    CHECK(!p);
}

TEST_CASE("unique_ptr move self-assignment of non-empty") {
    int *raw_p = new int(10);
    unique_ptr<int> p(raw_p);
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

TEST_CASE("unique_ptr reset") {
    auto test = [](int orig_non_empty) {
        unique_ptr<int> p(orig_non_empty == 1 ? new int(10) : nullptr);
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
    SUBCASE("orig_non_empty is 0") {
        test(0);
    }
    SUBCASE("orig_non_empty is 1") {
        test(1);
    }
}

TEST_CASE("unique_ptr release from empty") {
    unique_ptr<int> p;
    CHECK(p.release() == nullptr);
    CHECK(!p);
    CHECK(p.get() == nullptr);
}

TEST_CASE("unique_ptr release from non-empty") {
    int *raw_p = new int(10);
    unique_ptr<int> p(raw_p);
    CHECK(p.release() == raw_p);
    delete raw_p;

    CHECK(!p);
    CHECK(p.get() == nullptr);
}

TEST_CASE("unique_ptr swap") {
    int *raw_p1 = new int(10);
    int *raw_p2 = new int(20);
    unique_ptr<int> p1(raw_p1);
    unique_ptr<int> p2(raw_p2);
    swap(p1, p2);

    CHECK(p1);
    CHECK(p2);
    CHECK(p1.get() == raw_p2);
    CHECK(p2.get() == raw_p1);
}

TEST_CASE("unique_ptr operator== and operator!=") {
    const unique_ptr<int> null1;
    const unique_ptr<int> null2;
    const unique_ptr<int> p1(new int(10));
    const unique_ptr<int> p2(new int(10));

    CHECK(null1 == null1);

    CHECK(null1 == null2);
    CHECK(null1 != p1);
    CHECK(null1 != p2);

    CHECK(null2 == null1);
    CHECK(p1 != null1);
    CHECK(p2 != null1);

    CHECK(p1 == p1);
    CHECK(p2 == p2);
    CHECK(p1 != p2);
    CHECK(p2 != p1);
}

}  // namespace

#endif  // TEST_UNIQUE_PTR

// NOLINTEND(misc-use-anonymous-namespace)
// NOLINTEND(readability-function-cognitive-complexity)
