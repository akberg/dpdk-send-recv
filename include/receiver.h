#pragma once

#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_eal.h>
#include <rte_atomic.h>

#include "common.h"

struct receiver_args_t {
    int burst_size;
    rte_atomic64_t *rx_counter;   // Shared memory to count 
};

/**
 * Listen on port, and print then drop any received packets
 */
static void lcore_recv(rte_atomic64_t *rx_counter);


__rte_noreturn int lcore_receiver_main(void *args);
