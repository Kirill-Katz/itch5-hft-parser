#include "benchmarks/benchmark_utils.hpp"

#include <fstream>
#include <iostream>
#include <x86intrin.h>

pid_t run_perf_report() {
    pid_t pid = fork();
    if (pid == 0) {
        char pidbuf[32];
        snprintf(pidbuf, sizeof(pidbuf), "%d", getppid());

        execlp(
            "perf",
            "perf",
            "record",
            "-F",
            "999",
            "-g",
            "-p", pidbuf,
            nullptr
        );

        _exit(127);
    }

    return pid;
}

pid_t run_perf_stat() {
    pid_t pid = fork();
    if (pid == 0) {
        char pidbuf[32];
        snprintf(pidbuf, sizeof(pidbuf), "%d", getppid());

        execlp(
            "perf",
            "perf",
            "stat",
            "-p", pidbuf,
            nullptr
        );

        _exit(127);
    }

    return pid;
}

uint64_t calibrate_tsc() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    uint64_t t0_ns = ts.tv_sec * 1'000'000'000ull + ts.tv_nsec;

    unsigned aux;
    uint64_t c0 = __rdtscp(&aux);

    timespec sleep_ts;
    sleep_ts.tv_sec = 1;
    sleep_ts.tv_nsec = 0;

    nanosleep(&sleep_ts, nullptr);

    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    uint64_t t1_ns = ts.tv_sec * 1'000'000'000ull + ts.tv_nsec;

    uint64_t c1 = __rdtscp(&aux);

    uint64_t delta_cycles = c1 - c0;
    uint64_t delta_ns = t1_ns - t0_ns;

    __int128 tmp = (__int128)delta_cycles * 1'000'000'000;
    return (tmp + delta_ns / 2) / delta_ns;
}

void export_prices_csv(
    const std::vector<uint32_t>& prices,
    std::string outdir
) {
    std::vector<uint32_t> data = prices;

    std::ofstream out(outdir + "prices.csv");
    if (!out) {
        std::abort();
    }

    out << "price\n";
    for (uint32_t price : data) {
        out << price << "\n";
    }
}
