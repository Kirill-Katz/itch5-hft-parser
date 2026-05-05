#pragma once

#include <cstdint>
#include <iostream>

namespace OB {
struct Level {
    uint64_t qty;
    uint32_t price;
};

enum class Side : char {
    None = 0,
    Bid = 'B',
    Ask = 'S'
};

struct Order {
    uint32_t qty;
    uint32_t price;
    Side side;
};

struct BestLvlChange {
    uint64_t qty;
    uint32_t price;
    Side side;
};

[[gnu::cold, gnu::noinline]]
static void abort_unexpected(std::string_view message) {
    std::cerr << message << '\n';
    std::abort();
}

static inline void UNEXPECTED(bool condition, std::string_view message) {
    if (__builtin_expect(condition, false)) {
        abort_unexpected(message);
    }
}
}

