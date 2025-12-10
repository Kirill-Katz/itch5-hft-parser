#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <memory.h>
#include <chrono>
#include "parser.hpp"

inline uint64_t load_be48_v2(const std::byte* p) {
    uint64_t v;

    memcpy(&v, p, 6);
    v = __builtin_bswap64(v);
    return v >> 16;
}


class Handler {
public:
    void handle(ITCH::Message msg) {
        messages_num++;
        perCategory[msg.type]++;
    }

    std::unordered_map<ITCH::MessageType, int> perCategory;
    long long messages_num = 0;
};

int main() {
    std::ifstream file("../data/01302019.NASDAQ_ITCH50",
                       std::ios::binary | std::ios::in);

    if (!file) {
        std::cerr << "Failed to open file\n";
        return 1;
    }

    std::vector<std::byte> src_buf;
    try {
        src_buf.resize(1ull << 30);
    } catch (const std::bad_alloc&) {
        std::cerr << "Allocation failed\n";
        return 1;
    }

    file.read(reinterpret_cast<char*>(src_buf.data()), src_buf.size());
    std::size_t bytes_read = file.gcount();

    const std::byte* src = src_buf.data();

    Handler handler{};
    ITCH::ItchParser parser;
    size_t len = 1ull * 1024 * 1024 * 1024;

    using clock = std::chrono::high_resolution_clock;

    auto start = clock::now();

    parser.parse(src, len, handler);

    auto end = clock::now();

    uint64_t msgs = handler.messages_num;

    double seconds =
        std::chrono::duration_cast<std::chrono::duration<double>>(end - start).count();

    double throughput_msgs_per_sec = msgs / seconds;
    double avg_ns_per_msg = (seconds * 1e9) / msgs;

    for (auto& [key, value] : handler.perCategory) {
        std::cout << char(key) << ' ' << value << '\n';
    }

    std::cout << "Messages:        " << msgs << "\n";
    std::cout << "Seconds:         " << seconds << "\n";
    std::cout << "Throughput:      " << throughput_msgs_per_sec << " msg/s\n";
    std::cout << "Average latency: " << avg_ns_per_msg << " ns/msg\n";

    return 0;

}
