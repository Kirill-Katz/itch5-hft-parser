#pragma once
#include <iostream>

#include <absl/container/flat_hash_map.h>
#include "order_book_shared.hpp"

namespace OB {

template<Side S>
class MapLevel {
public:
    MapLevel() {
        levels_map.reserve(3000);
    }

    void remove(Level level);
    void add(Level level);
    Level best() const;

    absl::flat_hash_map<uint32_t, uint64_t> levels_map;
};

template<Side S>
inline Level MapLevel<S>::best() const {
    //return levels.back();
}

template<Side S>
inline void MapLevel<S>::remove(Level level) {
    auto target_level = levels_map.find(level.price);

    UNEXPECTED(target_level == levels_map.end(), "Remove didn't find a level");
    UNEXPECTED(level.qty > target_level->second, "Remove lead to an underflow for level");

    target_level->second -= level.qty;
    if (target_level->second == 0) {
        levels_map.erase(target_level);
    }
}

template<Side S>
inline void MapLevel<S>::add(Level level) {
    auto target_level = levels_map.find(level.price);

    if (target_level != levels_map.end()) {
        target_level->second += level.qty;
    } else {
        levels_map.insert({level.price, level.qty});
    }
}

}
