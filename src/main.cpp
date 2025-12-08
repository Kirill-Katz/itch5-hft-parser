#include <iostream>
#include <fstream>
#include <array>
#include <cstdint>
#include <memory.h>
#include "parser.hpp"

inline uint16_t load_be16(const std::byte* p) {
    return (uint16_t(p[0]) << 8) | uint16_t(p[1]);
}

inline uint64_t load_be48(const std::byte* p) {
    return (uint64_t(p[0]) << 40) |
           (uint64_t(p[1]) << 32) |
           (uint64_t(p[2]) << 24) |
           (uint64_t(p[3]) << 16) |
           (uint64_t(p[4]) << 8)  |
           uint64_t(p[5]);
}

inline uint64_t load_be48_v2(const std::byte* p) {
    uint64_t v;

    memcpy(&v, p, 6);
    v = __builtin_bswap64(v);
    return v >> 16;
}


class Handler {
public:
    void handle(Message msg) {
        std::cout << (char)msg.type << '\n';
    }
};

int main() {
    std::ifstream file("../data/01302019.NASDAQ_ITCH50",
                       std::ios::binary | std::ios::in);

    if (!file) {
        std::cerr << "Failed to open file\n";
        return 1;
    }

    std::array<std::byte, 4096> src_buf{};
    std::array<std::byte, 4096> dst_buf{};

    file.read(reinterpret_cast<char*>(src_buf.data()), src_buf.size());
    std::size_t bytes_read = file.gcount();

    if (bytes_read == 0) {
        std::cerr << "File empty or read failed\n";
        return 1;
    }

    const std::byte* src = src_buf.data();
    std::byte* dst = dst_buf.data();

    Handler handler{};
    ItchParser parser;
    parser.parse(src, 4096, handler);

    //const std::byte* msg = src;
    //uint16_t message_len = load_be16(msg);
    //char message_type = char(msg[2]);
    //uint16_t stock_locate = load_be16(msg + 3);
    //uint16_t tracking_num = load_be16(msg + 5);
    //uint64_t timestamp    = load_be48(msg + 7);
    //uint64_t timestamp2   = load_be48_v2(msg + 7);
    //char event_code       = char(msg[13]);

    //std::cout << message_len  << "\n";
    //std::cout << message_type << "\n";
    //std::cout << stock_locate << "\n";
    //std::cout << tracking_num << "\n";
    //std::cout << timestamp    << "\n";
    //std::cout << timestamp2   << "\n";
    //std::cout << event_code   << "\n";

    //uint16_t next_message_len = load_be16(msg + message_len + 2);
    //std::cout << next_message_len << "\n";
    //std::cout << char(msg[message_len + 2 + 2]) << "\n";

    return 0;
}
