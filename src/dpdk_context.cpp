#include "dpdk_context.hpp"

#include <cstdint>
#include <stdexcept>
#include <rte_ethdev.h>

void DPDKContext::setup_eal(int& argc, char**& argv) {
    int eal_argc = rte_eal_init(argc, argv);
    if (eal_argc < 0) {
        throw std::runtime_error("EAL init failed");
    }

    if (rte_eth_dev_count_avail() == 0) {
        throw std::runtime_error("Specify a vdev device");
    }

    argc -= eal_argc;
    argv += eal_argc;
}

void DPDKContext::setup_mempool() {
    constexpr uint32_t mbuf_count = 8191;
    constexpr uint32_t mbuf_cache_size = 256;

    pool_ = rte_pktmbuf_pool_create(
        "mbuf_pool",
        mbuf_count,
        mbuf_cache_size,
        0,
        RTE_PKTMBUF_HEADROOM + 2048,
        rte_socket_id()
    );

    if (!pool_) {
        throw std::runtime_error("mempool creation failed\n");
    }
}


void DPDKContext::setup_eth_device(uint16_t port_id) {
    rte_eth_dev_info dev_info;
    if (rte_eth_dev_info_get(port_id, &dev_info) != 0)
        throw std::runtime_error("dev info failed");

    uint16_t rx_desc = 1024;
    uint16_t tx_desc = 1024;
    if (rte_eth_dev_adjust_nb_rx_tx_desc(port_id, &rx_desc, &tx_desc) != 0)
        throw std::runtime_error("adjust nb desc failed");

    rte_eth_conf conf{};
    conf.txmode.offloads = 0;
    conf.rxmode.offloads = 0;

    if (rte_eth_dev_configure(port_id, 1, 1, &conf) < 0)
        throw std::runtime_error("dev configure failed");

    rte_eth_txconf txconf = dev_info.default_txconf;
    txconf.offloads = 0;

    rte_eth_rxconf rxconf = dev_info.default_rxconf;
    rxconf.offloads = 0;

    if (rte_eth_tx_queue_setup(port_id, 0, tx_desc,
                               rte_socket_id(), &txconf) != 0)
        throw std::runtime_error("tx queue failed");

    if (rte_eth_rx_queue_setup(port_id, 0, rx_desc,
                               rte_socket_id(), &rxconf, pool_) != 0)
        throw std::runtime_error("rx queue failed");

    if (rte_eth_dev_start(port_id) < 0)
        throw std::runtime_error("dev start failed");
}
