#pragma once

#include <iostream>
#include <string_view>

namespace test {

inline int& failureCount() {
    static int failures = 0;
    return failures;
}

inline void reportFailure(std::string_view expression, std::string_view file, int line) {
    ++failureCount();
    std::cerr << file << ':' << line << ": CHECK failed: " << expression << '\n';
}

inline int finish(std::string_view suiteName) {
    if (failureCount() == 0) {
        std::cout << suiteName << ": all tests passed\n";
        return 0;
    }

    std::cerr << suiteName << ": " << failureCount() << " check(s) failed\n";
    return 1;
}

}

// Unlike assert(), also runs in Release builds.
#define CHECK(expr)                                              \
    do {                                                         \
        if (!(expr)) {                                           \
            ::test::reportFailure(#expr, __FILE__, __LINE__);    \
        }                                                        \
    } while (false)

#define CHECK_THROWS(expr)                                       \
    do {                                                         \
        bool thrown_ = false;                                    \
        try {                                                    \
            (void)(expr);                                        \
        } catch (...) {                                          \
            thrown_ = true;                                      \
        }                                                        \
        if (!thrown_) {                                          \
            ::test::reportFailure(#expr " throws", __FILE__, __LINE__); \
        }                                                        \
    } while (false)
