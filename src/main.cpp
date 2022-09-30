#include <stdio.h>
#include <stdint.h>

#include <rte_version.h>    /* rte_version */
//#include <rte_memory.h>
#include <rte_launch.h>
#include <rte_eal.h>        /* rte_init */
#include <rte_per_lcore.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include <rte_debug.h>      /* rte_panic */
#include <rte_ethdev.h>     /* RTE_ETHER_MTU */
#include <rte_mbuf.h>
#include <rte_net.h>
#include <rte_udp.h>        /* udp_hdr */

#include "pkt_display.h"
    // rte_arp, rte_icmp, rte_ether
#include "sender.h"
    // rte_ether, rte_ethdev, common
#include "receiver.h"
    // rte_ether, rte_ethdev, common
#include "fizzbuzz.h"

// EAL setup -----------------------------------------------------------------//
#define PG_JUMBO_FRAME_LEN (9600 + RTE_ETHER_CRC_LEN + RTE_ETHER_HDR_LEN)
#ifndef RTE_JUMBO_ETHER_MTU
#define RTE_JUMBO_ETHER_MTU (PG_JUMBO_FRAME_LEN - RTE_ETHER_HDR_LEN - RTE_ETHER_CRC_LEN) /*< Ethernet MTU. */
#endif

static volatile uint8_t dpdk_quit_signal; 

static const struct rte_eth_conf port_conf_default = {
  .rxmode = {
    //.mq_mode = ETH_MQ_RX_NONE, // Deprecated
    .mtu = 9000,
    .max_lro_pkt_size = 9000,
    .split_hdr_size = 0,
    //.offloads = DEV_RX_OFFLOAD_JUMBO_FRAME,
  },

  .txmode = {
    .offloads = (RTE_ETH_TX_OFFLOAD_MULTI_SEGS),
  },
};

/**
 * Configure port with default settings
 * 
 * Initializes the given port using global settings and with the RX buffers
 * coming from the mbuf_pool passed as a parameter
 */
static inline int port_init(const uint16_t port, const uint16_t rx_rings, 
    const uint16_t tx_rings, rte_mempool *mbuf_pool)
{
    rte_eth_conf        port_conf   = port_conf_default;
    uint16_t            nb_rxd      = RX_RING_SIZE * 2;
    uint16_t            nb_txd      = TX_RING_SIZE * 2;
    int                 ret;
    uint16_t            q;
    rte_eth_dev_info    dev_info;
    rte_eth_txconf      txconf;
    rte_ether_addr      addr;

    if (!rte_eth_dev_is_valid_port(port)) return -1;

    // Clear memory
    memset(&port_conf, 0, sizeof(struct rte_eth_conf));

    // Get device information
    ret = rte_eth_dev_info_get(port, &dev_info);
    if (ret) {
        printf("Error during getting device (port %u) info: %s\n", 
            port, strerror(-ret));
        return ret;
    }

    if (dev_info.tx_offload_capa & RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE) {
        port_conf.txmode.offloads |= RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE;
    }

    // Configure Ethernet device
    ret = rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf);
    if (ret) return ret;

    // Set MTU to allow jumboframes
    ret = rte_eth_dev_set_mtu(port, PG_JUMBO_FRAME_LEN);
    if (ret) return ret;

    ret = rte_eth_dev_adjust_nb_rx_tx_desc(port, &nb_rxd, &nb_txd);
    if (ret) return ret;

    // Allocate and set up 1 RX queue per Ethernet port
    for (q = 0; q < rx_rings; q++) {
        ret = rte_eth_rx_queue_setup(
            port, q, nb_rxd, rte_eth_dev_socket_id(port), NULL, &mbuf_pool[q]
            );
        if (ret < 0) return ret;
    }

    txconf = dev_info.default_txconf;
    txconf.offloads = port_conf.txmode.offloads;

    // Allocate and set up 1 TX queue per Ethernet port
    for (q = 0; q < tx_rings; q++) {
        ret = rte_eth_tx_queue_setup(
            port, q, nb_txd, rte_eth_dev_socket_id(port), &txconf
            );
        if (ret < 0) return ret;
    }

    // Starting Ethernet port
    ret = rte_eth_dev_start(port);
    if (ret < 0) return ret;

    ret = rte_eth_macaddr_get(port, &addr);
    if (ret) return ret;

    // Display the port MAC address
    printf("Port %u MAC: " RTE_ETHER_ADDR_PRT_FMT "\n",
			port, RTE_ETHER_ADDR_BYTES(&addr));
    
    // Enable RX in promiscuous mode for the Ethernet device
    ret = rte_eth_promiscuous_enable(port);
    if (ret) return ret;

    return 0;
}


/**
 * RTE hello world
 */
static int lcore_hello(__rte_unused void *arg)
{
    unsigned lcore_id;
    lcore_id = rte_lcore_id();
    printf("hello from core %u\n", lcore_id);
    return 0;
}

int main(int argc, char **argv)
{
    int ret;
    int lcore_id;
    unsigned nb_ports;
    uint16_t port_id;
    rte_mempool *mbuf_pool;
    unsigned int lcore_count = rte_lcore_count();

    // Initialize EAL with cmdline arguments
    ret = rte_eal_init(argc, argv);
    if (ret < 0) {
        rte_panic("Cannot init EAL. Are you running the application as root?\n");
    }

    // Advance arg pointer
    argc -= ret;
    argv += ret;

    printf("%s\n", rte_version());

    printf("RTE_MAX_ETHPORTS = %i\n", RTE_MAX_ETHPORTS);
    printf("RTE_MAX_LCORE = %i\n", RTE_MAX_LCORE);
    // printf("RTE_JUMBO_ETHER_MTU = %i\n", RTE_JUMBO_ETHER_MTU);
    printf("RTE_ETHER_MTU = %i\n", RTE_ETHER_MTU);
    printf("RTE_CACHE_LINE_SIZE = %i\n", RTE_CACHE_LINE_SIZE);
    printf("RTE_MEMPOOL_CACHE_MAX_SIZE = %i\n", RTE_MEMPOOL_CACHE_MAX_SIZE);
    printf("SOCKET_ID_ANY = %i\n", SOCKET_ID_ANY);
    // printf("DEFAULT_MBUF_SIZE = %i\n", DEFAULT_MBUF_SIZE);
    // printf("PG_ETHER_MAX_JUMBO_FRAME_LEN = %i\n", PG_ETHERMAX_JUMBO_FRAME_LEN);
    printf("RTE_PKTBUF_HEADROOM = %i\n", RTE_PKTMBUF_HEADROOM);

    // Browse all running lcores except main core and launch function
    RTE_LCORE_FOREACH_WORKER(lcore_id) {
        rte_eal_remote_launch(lcore_hello, NULL, lcore_id);
    }

    //lcore_hello(NULL);

    // Count available devices
    // - TX: 1 port on ConnectX-5
    // - RX: 2 ports on BF2
    nb_ports = rte_eth_dev_count_avail();
    printf("nb_ports = %i\n", nb_ports);
    char port_0_name[64];
    rte_eth_dev_get_name_by_port(0, port_0_name);
    rte_eth_conf dev_conf;
    rte_eth_dev_conf_get(0, &dev_conf);
    rte_eth_dev_info dev_info;
    rte_eth_dev_info_get(0, &dev_info);
    printf("dev_info->device->name = %s\n", dev_info.device->name);
    printf("dev_info->device->driver->alias = %s\n", dev_info.device->driver->alias);
    printf("dev_info->driver_name = %s\n", dev_info.driver_name);
    printf("Port %i: %s\n", 0, port_0_name);
    rte_ether_addr mac_addr;
    rte_eth_macaddr_get(0, &mac_addr);
    printf("MAC: " RTE_ETHER_ADDR_PRT_FMT "\n", RTE_ETHER_ADDR_BYTES(&mac_addr));
    printf("Initializing:\n");
    nb_ports = 1;

    // Create a new mempool in memory to hold the mbufs

    // Allocate a mempool to hold the mbuf
    mbuf_pool = rte_pktmbuf_pool_create(
        "MBUF_POOL", 
        4 * NUM_MBUFS * nb_ports, 
        MBUF_CACHE_SIZE * 2, 
        0, 
        9800, //RTE_MBUF_DEFAULT_BUF_SIZE, // maximum RX packet length(9636) with head-room(128) = 9764
        rte_socket_id()
    );

    if (mbuf_pool == NULL) rte_panic("Cannot create mbuf pool\n");



    printf("%s\n", argv[1]);

    if (strcmp("-S", argv[1]) == 0) {    
        // Initialize port 0 -------------------------------------------------//
        ret = port_init(0, 1, 1, mbuf_pool);
        if (ret != 0) rte_panic("Cannot init port 0\n");

        { // scope
            uint16_t mtu;
            rte_eth_dev_get_mtu(port_id, &mtu);
            printf("MTU: %u\n", mtu);
        };
        // Execute sender ----------------------------------------------------//

        // Run on only one core
        // for (int i = 0; i < 1; i++)
        //     lcore_send(64, 1000000, mbuf_pool);

        // L cores
        // N burst size
        // P payload size
        // B message size 
        // 1 frame can hold P / B messages
        // each core sends N * P / B messages per burst
        // total of L * N * P / B messages sent at the time
        // rate is (ETH + IP + UDP + P) * N * L / delay:sec 
        
        sender_args_t *args = (sender_args_t *)malloc(sizeof(sender_args_t));
        args->mbuf_pool     = mbuf_pool;
        args->burst_size    = BURST_SIZE;
        args->delay         = 10000;
        args->payload_size  = sizeof(FizzbuzzPkt) * 390;
        printf("Payload size: %u bytes\n", args->payload_size);

        // Run on worker cores
        RTE_LCORE_FOREACH_WORKER(lcore_id) {
            rte_eal_remote_launch(lcore_sender_main, (void *)args, lcore_id);
        }
    } 
    else if (strcmp("-R", argv[1]) == 0) {
        // Initialize port 0 -------------------------------------------------//
        ret = port_init(0, 1, 1, mbuf_pool);
        if (ret != 0) rte_panic("Cannot init port 0\n");

        { // scope
            uint16_t mtu;
            rte_eth_dev_get_mtu(port_id, &mtu);
            printf("MTU: %u\n", mtu);
        };
        // Execute receiver --------------------------------------------------//

        // Run on only one core
        // lcore_recv(NULL);
    
        receiver_args_t *args = (receiver_args_t *)rte_malloc("receiver_args_t", sizeof(receiver_args_t), 0);
        //args->mbuf_pool     = mbuf_pool;
        args->burst_size    = BURST_SIZE;
        rte_atomic64_t rx_counters[lcore_count];
        printf("Counters: %x\n", rx_counters);

        RTE_LCORE_FOREACH_WORKER(lcore_id) {
            args->rx_counter = &(rx_counters[lcore_id]);
            rte_atomic64_set(args->rx_counter, 0);
            rte_eal_remote_launch(lcore_receiver_main, (void *)args, lcore_id);
        }

        // Report reception rate
        int64_t rx_step = 0, 
                rx_total = 0, 
                rate;
        char *prefixes[] = {"", "k", "M", "G"};
        for (;;) {
            rx_step = 0;
            RTE_LCORE_FOREACH_WORKER(lcore_id) {
                printf("counter addr: %x ", &(rx_counters[lcore_id]));
                int c = rte_atomic64_read(&(rx_counters[lcore_id]));
                printf("count: %u\n", c);
                rx_step += c;
            }
            int64_t delta = 8 * (rx_step - rx_total) / (REPORT_INTERVAL_us / 1000000);
            int p;
            for (p = 0; p < 4; p++) {
                if (delta > 1000) {
                    delta /= 1000;
                } else { break; }
            }
            printf("rate: %llu %sbps total: %llu\n", delta, prefixes[p], rx_total);
            rx_total = rx_step;

            rte_delay_us_sleep(REPORT_INTERVAL_us);
        }
    }
    else {
        // Initialize port 0 -------------------------------------------------//
        printf("Available devices: %u\n", rte_eth_dev_count_avail());
        return 0;
        ret = port_init(0, 1, 1, mbuf_pool);
        if (ret != 0) rte_panic("Cannot init port 0\n");

        { // scope
            uint16_t mtu;
            rte_eth_dev_get_mtu(port_id, &mtu);
            printf("MTU: %u\n", mtu);
        };
        // Execute forwarder -------------------------------------------------//

    
        receiver_args_t *args = (receiver_args_t *)rte_malloc("receiver_args_t", sizeof(receiver_args_t), 0);
        //args->mbuf_pool     = mbuf_pool;
        args->burst_size    = BURST_SIZE;
        rte_atomic64_t rx_counters[lcore_count];
        // rte_atomic64_t *rx_counters = (rte_atomic64_t *)rte_malloc(
        //     "rte_atomic64_t", sizeof(rte_atomic64_t) * lcore_count, 0);
        printf("Counters: %x\n", rx_counters);
        // TODO: Shared memory counter to count rx rate

        RTE_LCORE_FOREACH_WORKER(lcore_id) {
            args->rx_counter = &(rx_counters[lcore_id]);
            rte_atomic64_set(args->rx_counter, 0);
            rte_eal_remote_launch(lcore_receiver_main, (void *)args, lcore_id);
        }

        // Report reception rate
        int64_t rx_step = 0, 
                rx_total = 0, 
                rate;
        char *prefixes[] = {"", "k", "M", "G"};
        for (;;) {
            rx_step = 0;
            RTE_LCORE_FOREACH_WORKER(lcore_id) {
                printf("counter addr: %x ", &(rx_counters[lcore_id]));
                int c = rte_atomic64_read(&(rx_counters[lcore_id]));
                printf("count: %u\n", c);
                rx_step += c;
            }
            int64_t delta = (rx_step - rx_total) / (REPORT_INTERVAL_us / 1000000);
            int p;
            for (p = 0; p < 4; p++) {
                if (delta > 1000) {
                    delta /= 1000;
                } else { break; }
            }
            printf("rate: %llu %sbps total: %llu\n", delta, prefixes[p], rx_total);
            rx_total = rx_step;
            rte_delay_us_sleep(REPORT_INTERVAL_us);
        }

    }

    // Wait for all lcores to finish
    rte_eal_mp_wait_lcore();
    rte_eal_cleanup();
    return 0;
}