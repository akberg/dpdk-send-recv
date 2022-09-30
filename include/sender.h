#pragma once

#include <rte_ethdev.h>

#include "common.h"

#define MAX_UDP_PKT_SIZE 1480

struct sender_args_t {
    rte_mempool *mbuf_pool;
    int payload_size;
    int delay;
    int burst_size;
};

/**
 * Send a packet with payload size pkt_size every delay microseconds
 */
void lcore_send(uint16_t payload_size, int burst_size, rte_mempool *mbuf_pool);

/**
 * Sender main function
 */
__rte_noreturn int lcore_sender_main(void *args);