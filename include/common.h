#pragma once

#include <stdint.h>
#include <rte_malloc.h> /* rte_malloc */
#include <rte_ip.h>     /* RTE_IPV4() */

// DPDK definitions ----------------------------------------------------------//
#define RX_RING_SIZE    1024
#define TX_RING_SIZE    1024

#define NUM_MBUFS       8191
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE      32

// ---------------------------------------------------------------------------//
#define COMMON_IP_DST       RTE_IPV4(15,15,15,7)
#define COMMON_IP_SRC       RTE_IPV4(15,15,15,6)

#define IPV4_MIN_HDR_LEN    20
#define UDP_HDR_LEN         8

#define REPORT_INTERVAL_us  2000000

/**
 * Example message as UDP payload, dynamic packet size.
 */
struct msg_t {
    uint16_t seq_nb;
    uint16_t id;
    uint16_t data_len;
    char data;

    /**
     * Create a variable sized msg_t object
     */
    __rte_always_inline
    static msg_t *create(const uint16_t payload_size) {
        size_t msg_size = 2*3+payload_size;
        msg_t * msg = (msg_t *)rte_malloc("msg_t", msg_size, 0);
        memset(msg, 0, msg_size);
        msg->data_len = payload_size;
        return msg;
    }
    /**
     * Get size of msg_t object.
     */
    __rte_always_inline
    size_t size() { return sizeof(uint16_t) * 3 + this->data_len; }
};

#define MSG_HDR_LEN     (2 * 3)
#define MSG_PRT_FMT     "msg_t { seq_nb = 0x%04x, id = %u, "\
                        "data_len = %u, data = %s }"
#define MSG_FIELDS(m)   ((m).seq_nb), ((m).id), ((m).data_len), (&((m).data))
