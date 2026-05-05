#include <iostream>

#include <rte_ether.h>
#include <rte_ip4.h>
#include <rte_udp.h>
#include <vector>
#include <iostream>
#include <time.h>
#include <rte_eal.h>
#include <rte_ethdev.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>
#include <thread>
#include <random>

#include "itch_parser.hpp"
#include "benchmarks/benchmark_utils.hpp"
#include "benchmarks/example_benchmark.hpp"
#include "benchmarks/example_benchmark_parsing.hpp"
#include "dpdk_context.hpp"
#include "ingestor.hpp"
#include "handler.hpp"
#include "spmc_queue.hpp"

int main(int argc, char** argv) {
    cpu_set_t main_cpuset;
    CPU_ZERO(&main_cpuset);
    CPU_SET(1, &main_cpuset);
    int main_pin_result = pthread_setaffinity_np(
        pthread_self(),
        sizeof(cpu_set_t),
        &main_cpuset
    );
    if (main_pin_result != 0) {
        std::cerr << "Failed to pin main thread to core 2\n";
        return 1;
    }

    constexpr uint16_t port_id = 0;
    DPDKContext dpdk_context(port_id);

    dpdk_context.setup_eal(argc, argv);
    dpdk_context.setup_mempool();
    dpdk_context.setup_eth_device(port_id);

    std::string outdir;

    if (argc != 2) {
        std::cout << "Please specify the file to parse and an output directory" << '\n';
        return 1;
    }

    outdir = argv[1];

    ITCH::ItchParser parser;
    BenchmarkOrderBook ob_bm_handler;
    BenchmarkParsing parsing_bm_handler;

    std::vector<Handler::InstrumentConfig> instrument_config;
    Handler::Queue nvda_queue;
    Handler::Queue aapl_queue;
    auto nvda_consumer = nvda_queue.make_consumer();
    auto aapl_consumer = aapl_queue.make_consumer();

    std::thread nvda_thread([c = std::move(nvda_consumer), o = outdir] () mutable {
        absl::flat_hash_map<uint64_t, uint64_t> latency_distribution;

        while (true) {
            StrategyMsg msg;
            unsigned aux_end;

            while (!c.pop(msg)) {}
            if (msg.type == StrategyMsgType::Stop) {
                break;
            }

            _mm_lfence();
            uint64_t t1 = __rdtscp(&aux_end);
            auto cycles = t1 - msg.book_update.t0;

            latency_distribution[cycles]++;
        }

        export_latency_distribution_csv_cycles(
            latency_distribution,
            o + "parsing_and_order_book_latency_distribution_strategy_nvda.csv"
        );
    });

    std::thread aapl_thread([c = std::move(aapl_consumer), o = outdir] () mutable {
        absl::flat_hash_map<uint64_t, uint64_t> latency_distribution;

        while (true) {
            StrategyMsg msg;
            unsigned aux_end;

            while (!c.pop(msg)) {}
            if (msg.type == StrategyMsgType::Stop) {
                break;
            }

            _mm_lfence();
            uint64_t t1 = __rdtscp(&aux_end);
            auto cycles = t1 - msg.book_update.t0;

            latency_distribution[cycles]++;
        }

        export_latency_distribution_csv_cycles(
            latency_distribution,
            o + "parsing_and_order_book_latency_distribution_strategy_aapl.csv"
        );
    });

    cpu_set_t nvda_cpuset;
    CPU_ZERO(&nvda_cpuset);
    CPU_SET(2, &nvda_cpuset);
    int nvda_pin_result = pthread_setaffinity_np(
        nvda_thread.native_handle(),
        sizeof(cpu_set_t),
        &nvda_cpuset
    );
    if (nvda_pin_result != 0) {
        std::cerr << "Failed to pin NVDA consumer thread to core 2\n";
        return 1;
    }

    cpu_set_t aapl_cpuset;
    CPU_ZERO(&aapl_cpuset);
    CPU_SET(4, &aapl_cpuset);
    int aapl_pin_result = pthread_setaffinity_np(
        aapl_thread.native_handle(),
        sizeof(cpu_set_t),
        &aapl_cpuset
    );
    if (aapl_pin_result != 0) {
        std::cerr << "Failed to pin AAPL consumer thread to core 4\n";
        return 1;
    }

    instrument_config.push_back({ .symbol = "NVDA", .queue = &nvda_queue });
    instrument_config.push_back({ .symbol = "AAPL", .queue = &aapl_queue });
    Handler handler(instrument_config);

    ITCH::Ingestor<Handler> ingestor(handler, dpdk_context);
    ingestor.ingest_messages();

    nvda_thread.join();
    aapl_thread.join();
    return 0;
}
