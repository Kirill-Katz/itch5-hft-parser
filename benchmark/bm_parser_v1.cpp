#include "parser_v1.hpp"
#include "utils.hpp"
#include <benchmark/benchmark.h>

struct HandlerV1 {
    uint64_t messages = 0;

    void handle(const ITCHv1::Message& msg) {
        messages++;
    }
};

static void BM_Parse_v1(benchmark::State& state) {
    static std::vector<std::byte> buf = load_chunk(3 * 1024 * 1024);

    ITCHv1::ItchParser parser;
    uint64_t messages = 0;

    for (auto _ : state) {
        HandlerV1 handler{};
        ITCHv1::Message msg = parser.parse_msg(buf.data());
        benchmark::DoNotOptimize(msg);
    }

    //state.counters["msg/s"] =
    //    benchmark::Counter(messages, benchmark::Counter::kIsRate);

    //state.counters["ns/msg"] =
    //benchmark::Counter(double(messages) / 1e9,
    //                   benchmark::Counter::kIsRate | benchmark::Counter::kInvert);
}

BENCHMARK(BM_Parse_v1);

