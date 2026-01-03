#pragma once
#include <vector>
#include <cstring>
#include "order_book_shared.hpp"

namespace OB {

template<Side S>
class VectorLevelSplit {
public:
    VectorLevelSplit() {
        qtys.reserve(3000);
        prices.reserve(3000);
    }

    void remove(Level level);
    void add(Level level);
    Level best() const;

    std::vector<uint64_t> qtys;
    std::vector<uint32_t> prices;
};


template<Side S>
inline Level VectorLevelSplit<S>::best() const {
    return Level{
        .price = prices.back(),
        .qty   = qtys.back()
    };
}

template<Side S>
inline void VectorLevelSplit<S>::remove(Level level) {
    auto it = std::lower_bound(
        prices.begin(), prices.end(), level.price,
        [](uint32_t lhs, uint32_t rhs) {
            if constexpr (S == Side::Bid) {
                return lhs < rhs;
            } else {
                return lhs > rhs;
            }
        }
    );

    UNEXPECTED(it == prices.end() || *it != level.price,
               "Remove didn't find a level");

    size_t idx = it - prices.begin();

    UNEXPECTED(level.qty > qtys[idx],
               "Remove lead to an underflow for level");

    qtys[idx] -= level.qty;
    if (qtys[idx] == 0) {
        prices.erase(prices.begin() + idx);
        qtys.erase(qtys.begin() + idx);
    }
}

template<Side S>
inline void VectorLevelSplit<S>::add(Level level) {
    auto it = std::lower_bound(
        prices.begin(), prices.end(), level.price,
        [](uint32_t lhs, uint32_t rhs) {
            if constexpr (S == Side::Bid) {
                return lhs < rhs;
            } else {
                return lhs > rhs;
            }
        }
    );

    size_t idx = it - prices.begin();

    if (it != prices.end() && *it == level.price) {
        qtys[idx] += level.qty;
    } else {
        prices.insert(it, level.price);
        qtys.insert(qtys.begin() + idx, level.qty);
    }
}

}
