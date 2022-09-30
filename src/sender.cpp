#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_malloc.h>
#include "sender.h"
#include "fizzbuzz.h"

uint16_t ip_cksum(rte_ipv4_hdr *ip_hdr)
{
    uint16_t *ptr16 = (unaligned_uint16_t*) ip_hdr;
    uint32_t ip_cksum = ptr16[0]
        + ptr16[1] + ptr16[2]
        + ptr16[3] + ptr16[4]
        + ptr16[5] + ptr16[6]
        + ptr16[7] + ptr16[8] + ptr16[9];
    // Reduce 32 bit checksum to 16 bits and complement it
    ip_cksum = ((ip_cksum & 0xffff0000) >> 16) + (ip_cksum & 0x0000ffff);
    if (ip_cksum > 65535) {
        ip_cksum -= 65535;
    }
    ip_cksum = (~ip_cksum) & 0x0000ffff;
    if (ip_cksum == 0) {
        ip_cksum = 0xffff;
    }
    return (uint16_t)ip_cksum;
}

void lcore_send(int iteration, uint16_t payload_size, int burst_size, rte_mempool *mbuf_pool)
{
    static uint8_t seq_nb = 0;
    int start_n;
    rte_ether_addr mac_src_addr;
    rte_ether_addr mac_dst_addr;
    uint16_t port = 0;

    uint32_t lcore_id       = rte_lcore_id();
    uint32_t lcore_count    = rte_lcore_count();

    rte_mbuf *bufs[burst_size];
    uint16_t ether_type     = RTE_ETHER_TYPE_IPV4;

    // P2P connection to this interface on Bluefield
    // TODO: Consult ARP table for destination
    mac_dst_addr = (rte_ether_addr){0xb8, 0xce, 0xf6, 0x61, 0x9f, 0x8a};
    // Don't remember where this address came from, switch maybe?
    // mac_src_addr = (rte_ether_addr){0xb9, 0xce, 0xf6, 0x61, 0x9f, 0x8a};

    // Get this port's MAC as src address
    rte_eth_macaddr_get(port, &mac_src_addr);
    // mac_src_addr = (rte_ether_addr){0x98, 0x03, 0x9b, 0x79, 0x9a, 0x20};

    rte_ether_hdr *eth_hdr;
    rte_ipv4_hdr *ip_hdr;
    // TX host IP
    uint32_t ip_dst_addr = COMMON_IP_DST;
    // RX host IP
    uint32_t ip_src_addr = COMMON_IP_SRC;


    // Build payload
    msg_t *msg = msg_t::create(payload_size);
    msg->id = rte_lcore_id();
    int n_fb = payload_size / sizeof(FizzbuzzPkt);
    
    // for (int i = 0; i < payload_size-1; i++) {
    //     (&(msg->data))[i] = 'a' + i % 26;
    // }

    start_n = n_fb * burst_size * (lcore_id - 1) + iteration * n_fb * lcore_count;
    if (iteration == 0) {
        printf("core %u starts at %u\n", lcore_id, start_n);
    }

    int pkt_size;

    for (int i = 0; i < burst_size; i++) {
        // Allocate buffer
        bufs[i] = rte_pktmbuf_alloc(mbuf_pool);

        // Ethernet header ---------------------------------------------------//
        eth_hdr = rte_pktmbuf_mtod(bufs[i], rte_ether_hdr *);
        eth_hdr->dst_addr   = mac_dst_addr;
        eth_hdr->src_addr   = mac_src_addr;
        eth_hdr->ether_type = rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4);

        pkt_size = sizeof(rte_ether_hdr);

        // IP header ---------------------------------------------------------//
        ip_hdr  = rte_pktmbuf_mtod_offset(bufs[i], rte_ipv4_hdr*, pkt_size);
        ip_hdr->version         = 4;    // Version and ihl 4 bits each,
        ip_hdr->ihl             = 5;    // covered by macros in definition
        // version_ihl is union with (version, ihl)
        // Minimal IP header length = 20
        ip_hdr->total_length    = ip_hdr->ihl * 4 + sizeof(rte_udp_hdr) + payload_size; 
        ip_hdr->packet_id       = 0;    // Sequence number? Other than 0
        ip_hdr->fragment_offset = 0;    
        ip_hdr->time_to_live    = 42;   // Default TTL is 64
        ip_hdr->type_of_service = 0;    // a.k.a DSCP
        ip_hdr->next_proto_id   = 17;   // UDP protocol id
        ip_hdr->src_addr        = rte_cpu_to_be_32(ip_src_addr);
        ip_hdr->dst_addr        = rte_cpu_to_be_32(ip_dst_addr);
        // IP checksum
        ip_hdr->hdr_checksum    = ip_cksum(ip_hdr);
        // IHL is number of 32-bit words in ip header
        pkt_size += ip_hdr->ihl * 4; //sizeof(rte_ipv4_hdr);

        // UDP packet --------------------------------------------------------//
        // Size of UDP packet should then
        msg->seq_nb += 0x0100;
        //printf("Message size is %u bytes\n", msg->size());
        rte_udp_hdr *udp_hdr;
        udp_hdr = rte_pktmbuf_mtod_offset(bufs[i], rte_udp_hdr *, pkt_size);
        pkt_size += sizeof(rte_udp_hdr);
        udp_hdr->dgram_len = msg->size();
        msg_t *msg_ptr     = rte_pktmbuf_mtod_offset(bufs[i], msg_t*, pkt_size);
        pkt_size       += msg->size();
        bufs[i]->data_len   = pkt_size;
        bufs[i]->pkt_len    = pkt_size;
        bufs[i]->nb_segs    = 1;

        // Fizzbuzz packet ---------------------------------------------------//
        for (int j = 0; j < n_fb; j++) {
            ((FizzbuzzPkt *)(&(msg->data)))[j] = FizzbuzzPkt(i*n_fb + j + start_n);
            //((FizzbuzzPkt *)(&(msg->data)))[j].play();
            // printf("(%u) %u: %s\n", lcore_id, ((FizzbuzzPkt *)(&(msg->data)))[j].seq_nb, ((FizzbuzzPkt *)(&(msg->data)))[j].data);
        }
        rte_memcpy(msg_ptr, msg, msg->size());

        // printf("Send packet (%u bytes)\n", bufs[i]->pkt_len);
        //printf(MSG_PRT_FMT "\n", MSG_FIELDS(*msg));
        // uint8_t *data = rte_pktmbuf_mtod(bufs[i], uint8_t *);
        // for (int j = 0; j < bufs[i]->pkt_len && ; j += 16) {
        //     for (int k = 0; k < 16 && k+j < bufs[i]->pkt_len; k++) {
        //         printf("%02x ", (data[j+k]));
        //     }
        //     printf("\n");
        // }
    }
    iteration++;

    int nb_tx = rte_eth_tx_burst(port, 0, bufs, burst_size);
    //printf("%i packets sent\n", nb_tx);
    for (int i = nb_tx; i < burst_size; i++) {
        rte_pktmbuf_free(bufs[i]);
    }
}

/**
 * Sender loop.
 */
__rte_noreturn int lcore_sender_main(void *args)
{
    sender_args_t *arg = (sender_args_t *)args;

    int burst_size          = arg->burst_size;
    int delay               = arg->delay;
    rte_mempool *mbuf_pool  = arg->mbuf_pool;
    int payload_size        = arg->payload_size;
    uint32_t lcore_count    = rte_lcore_count();
    int iteration           = 0;

    for (int i = 2460; true; i++) {
        lcore_send(iteration, payload_size, burst_size, mbuf_pool);
        iteration ++;
        rte_delay_us_sleep(delay);

    }
    for (;;) {}
    
}
