#include <array>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>

#include <sys/types.h>
#include <sys/wait.h>
#include <papi.h>
#include <unistd.h>

#include "parser_v1.hpp"
#include "parser_v2.hpp"
#include "array_level.hpp"
#include "order_book_handler_single.hpp"

struct CounterHandler {
    void handle(const ITCHv1::Message& msg) {
        messages_num++;
        counts[static_cast<unsigned char>(msg.type)]++;
    }

    void reset() {
        messages_num = 0;
        counts.fill(0);
    }

    std::array<uint64_t, 256> counts{};
    uint64_t messages_num = 0;
};

static void run_one(
    const char* name,
    ITCHv1::ItchParser& parser,
    CounterHandler& handler,
    const std::byte* src,
    size_t len
) {
    using clock = std::chrono::high_resolution_clock;

    handler.reset();

    auto start = clock::now();
    parser.parse(src, len, handler);
    auto end = clock::now();

    double seconds =
        std::chrono::duration_cast<std::chrono::duration<double>>(end - start).count();

    uint64_t msgs = handler.messages_num;
    double throughput = msgs / seconds;
    double avg_ns = (seconds * 1e9) / double(msgs);

    std::cout << "=== " << name << " ===\n";
    std::cout << "Messages:        " << msgs << "\n";
    std::cout << "Seconds:         " << seconds << "\n";
    std::cout << "Throughput:      " << throughput << " msg/s\n";
    std::cout << "Average latency: " << avg_ns << " ns/msg\n\n";
}

template<typename Handler>
static void dump_counts(const Handler& handler) {
    for (size_t i = 0; i < handler.counts.size(); ++i) {
        if (handler.counts[i] == 0) continue;
        std::cout << char(i) << ' ' << handler.counts[i] << '\n';
    }
}

std::pair<std::vector<std::byte>, size_t> init_benchmark() {
    std::ifstream file("../data/01302019.NASDAQ_ITCH50", std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file\n";
        return {};
    }

    std::vector<std::byte> src_buf;
    src_buf.resize(1LL * 1024 * 1024 * 1024);

    file.read(reinterpret_cast<char*>(src_buf.data()), src_buf.size());
    size_t bytes_read = size_t(file.gcount());

    if (bytes_read < 3) {
        std::cerr << "File read too small\n";
        return {};
    }

    return {src_buf, bytes_read};
}

pid_t run_perf() {
    pid_t pid = fork();
    if (pid == 0) {
        char pidbuf[32];
        snprintf(pidbuf, sizeof(pidbuf), "%d", getppid());

        execlp(
            "perf",
            "perf",
            "record",
            "-p", pidbuf,
            nullptr
        );

        _exit(127);
    }

    return pid;
}

void print_order_book(OB::OrderBook<OB::ArrayLevel> order_book) {
    std::cout << "====== Bid ======" << '\n';
    for (auto bid : order_book.bid_levels.levels) {
        std::cout << bid.price << ' ' << bid.qty << '\n';
    }

    std::cout << "====== Ask ======\n";
    for (auto it = order_book.ask_levels.levels.rbegin();
         it != order_book.ask_levels.levels.rend();
         ++it) {
        std::cout << it->price << ' ' << it->qty << '\n';
    }
}

void export_latency_distribution_csv(
    const absl::flat_hash_map<uint64_t, uint64_t>& latency_distribution
) {
    std::vector<std::pair<uint64_t, uint64_t>> data;
    data.reserve(latency_distribution.size());

    for (const auto& kv : latency_distribution) {
        data.emplace_back(kv.first, kv.second);
    }

    std::sort(
        data.begin(),
        data.end(),
        [](const auto& a, const auto& b) {
            return a.first < b.first;
        }
    );

    std::ofstream out("../data/latency_distribution.csv");
    if (!out) {
        std::abort();
    }

    out << "latency_ns,count\n";
    for (const auto& [latency, count] : data) {
        out << latency << "," << count << "\n";
    }
}

int main() {
    auto res = init_benchmark();
    auto src_buf = res.first;
    auto bytes_read = res.second;

    const std::byte* src = src_buf.data();
    size_t len = bytes_read;

    ITCHv1::ItchParser parser_v1;
    CounterHandler h1;
    run_one("ITCH v1", parser_v1, h1, src, len);

    pid_t perf_pid = run_perf();

    ITCHv2::ItchParser parser_v2;
    OrderBookHandlerSingle obHandler;
    parser_v2.parse_specific(src, len, obHandler);
    export_latency_distribution_csv(obHandler.latency_distribution);

    return 0;
}
