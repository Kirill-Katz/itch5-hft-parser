#include <benchmark/benchmark.h>
#include <fstream>
#include <vector>
#include <cstddef>
#include <iostream>

#include "parser.hpp"

__attribute__((noinline))
ITCH::Message call_parse(ITCH::ItchParser& parser, const std::byte* src) {
    return parser.parseMsg(src);
}

class Handler {
public:
    void handle(ITCH::Message msg) {
        messages_num++;
    }

    long long messages_num = 0;
};

static const std::vector<std::byte>& first_chunk()
{
    static std::vector<std::byte> buf = [] {
        std::ifstream file("../data/01302019.NASDAQ_ITCH50",
                           std::ios::binary);
        if (!file) {
            throw std::runtime_error("Failed to open ITCH file");
        }

        std::vector<std::byte> tmp(4096);
        file.read(reinterpret_cast<char*>(tmp.data()),
                  static_cast<std::streamsize>(tmp.size()));
        std::streamsize got = file.gcount();
        if (got <= 0) {
            throw std::runtime_error("Failed to read first chunk");
        }
        tmp.resize(static_cast<std::size_t>(got));
        return tmp;
    }();
    return buf;
}

static void BM_ParseMsg(benchmark::State& state) {
    static const std::vector<std::byte> buf = first_chunk();
    ITCH::ItchParser parser;

    const std::byte* vsrc = buf.data();
    ITCH::MessageType last_type = ITCH::MessageType::SYSTEM_EVENT;

    for (auto _ : state) {
        auto msg =  parser.parseMsg(vsrc);
        benchmark::DoNotOptimize(msg);
        last_type = msg.type;
    }

    std::cout << char(last_type) << '\n';
    state.SetItemsProcessed(state.iterations());
}

static void BM_Parse(benchmark::State& state) {
    std::ifstream file("../data/01302019.NASDAQ_ITCH50",
                   std::ios::binary | std::ios::in);

    if (!file) {
        std::cerr << "Failed to open file\n";
        return;
    }

    std::vector<std::byte> src_buf;
    try {
        src_buf.resize(1ull << 30);
    } catch (const std::bad_alloc&) {
        std::cerr << "Allocation failed\n";
        return;
    }

    file.read(reinterpret_cast<char*>(src_buf.data()), src_buf.size());
    std::size_t bytes_read = file.gcount();

    const std::byte* src = src_buf.data();

    ITCH::ItchParser parser;
    for (auto _ : state) {
        Handler handler{};
        parser.parse(src, 1 * 1024 * 1024 * 1024, handler);
    }

    state.SetBytesProcessed(1024 * 1024 * 1024);
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_Parse);
BENCHMARK(BM_ParseMsg);
