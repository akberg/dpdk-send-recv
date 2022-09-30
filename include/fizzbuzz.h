#pragma once

#include <stdint.h>
#include <rte_malloc.h>
#include <rte_lcore.h>

/**
 * Data length, maximum number of digits available to send
 */
#define FIZZBUZZ_MAX_DATA_LEN 16
#define FIZZ 3
#define BUZZ 7

struct FizzbuzzPkt {
    uint32_t src_core;
    uint32_t seq_nb;                    // Actual sequence number
    char data[FIZZBUZZ_MAX_DATA_LEN];   // String repr number, modifiable

    /* methods */

    FizzbuzzPkt(const uint32_t seq_nb);

    static FizzbuzzPkt *create(const uint32_t seq_nb);

    void play();

    bool verify();
};