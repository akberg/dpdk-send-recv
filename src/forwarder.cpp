/**
 * Filtering application running on SmartNIC
 * 
 * Receive packet from TX, do modifications or filtering, and forward to RX
 */
#include "forwarder.h"

#include <rte_eal.h>

#include "receiver.h"
#include "pkt_display.h"
#include "fizzbuzz.h"


static void lcore_fwrd(rte_atomic64_t *rx_counter)
{
    uint16_t port = 0;
    uint32_t lcore_id       = rte_lcore_id();
    uint32_t lcore_count    = rte_lcore_count();
    uint32_t hdrs_len       = RTE_ETHER_HDR_LEN + IPV4_MIN_HDR_LEN + 
                              UDP_HDR_LEN + MSG_HDR_LEN;

    for (;;) {
        rte_mbuf *bufs[BURST_SIZE];
        rte_mbuf *pkt;
        uint16_t nb_rx = rte_eth_rx_burst(port, 0, bufs, BURST_SIZE);
        rte_arp_hdr *arp_hdr;

        // Peek packets ------------------------------------------------------//
        for (int i = 0; i < nb_rx; i++) {
            int n_fb = (bufs[i]->pkt_len - hdrs_len) / sizeof(FizzbuzzPkt);
            printf("Received %u packets with (%i - %i) / %i = %i fb\n", nb_rx, bufs[i]->pkt_len, hdrs_len, sizeof(FizzbuzzPkt), n_fb);
            FizzbuzzPkt *fb = rte_pktmbuf_mtod_offset(bufs[i], FizzbuzzPkt *, hdrs_len);
            for (int j = 0; j < n_fb; j++) {
                printf("- %u: %s\n", fb[j].seq_nb, fb[j].data);
            }
            //printf("%u, lcore %u, packet %u, %u bytes, src core: %u\n", it, lcore_id, i, bufs[i]->pkt_len, fb->src_core);
            // fflush(stdout);
            rte_atomic64_add(rx_counter, bufs[i]->pkt_len);
            rte_pktmbuf_free(bufs[i]);
            // ! Count received packets and nothing more
            continue;
            int pkt_offset = 0;
            // mbuf info -----------------------------------------------------//
            printf("\n\n");
            printf("# mbuf\n");
            pkt = bufs[i];
            printf("packet_type = %08x\n", pkt->packet_type);
            printf("Stack:\nL2: %s\nL3: %s\nL4: %s\n", 
                rte_get_ptype_l2_name(pkt->packet_type),
                rte_get_ptype_l3_name(pkt->packet_type),
                rte_get_ptype_l4_name(pkt->packet_type)
                );

            // Ethernet packet size
            printf("pkt_len = %u\n", pkt->pkt_len);
            printf("data_len = %u\n", pkt->data_len);

            // Ethernet header -----------------------------------------------//
            // Ethernet add 18 bytes
            rte_ether_hdr *eth = rte_pktmbuf_mtod(pkt, rte_ether_hdr *);
            pkt_offset += print_eth_hdr(eth);
            printf("pkt_offset = %u\n", pkt_offset);
            uint16_t ether_type = rte_be_to_cpu_16(eth->ether_type);

            switch(ether_type) {
            case RTE_ETHER_TYPE_ARP: {
                // ARP request -----------------------------------------------//
                arp_hdr = rte_pktmbuf_mtod_offset(pkt, rte_arp_hdr *, pkt_offset);
                pkt_offset += print_arp_hdr(arp_hdr);
                printf("pkt_offset = %u\n", pkt_offset);
                break;
            }
            case RTE_ETHER_TYPE_IPV4: {
                // Unwrap IP frame -------------------------------------------//
                rte_ipv4_hdr *ip_hdr;
                ip_hdr = rte_pktmbuf_mtod_offset(pkt, rte_ipv4_hdr *, pkt_offset);
                pkt_offset += print_ipv4_hdr(ip_hdr);
                printf("pkt_offset = %u\n", pkt_offset);

                switch (ip_hdr->next_proto_id) {
                case IPPROTO_ICMP: {
                    // ICMP Header -------------------------------------------//
                    rte_icmp_hdr *icmp;
                    icmp = rte_pktmbuf_mtod_offset(pkt, rte_icmp_hdr *, pkt_offset);
                    pkt_offset += print_icmp_hdr(icmp);
                    printf("pkt_offset = %u\n", pkt_offset);
                    break;
                }
                case IPPROTO_UDP: {
                    // UDP packet --------------------------------------------//
                    rte_udp_hdr *udp_hdr;
                    udp_hdr = rte_pktmbuf_mtod_offset(pkt, rte_udp_hdr *, pkt_offset);
                    pkt_offset += sizeof(rte_udp_hdr);
                    printf("pkt_offset = %u\n", pkt_offset);

                    printf("UDP packet\n");
                    printf("dgram_len = %u\n", udp_hdr->dgram_len);
                    msg_t *udp_data;
                    udp_data = rte_pktmbuf_mtod_offset(pkt, msg_t *, pkt_offset);
                    printf(MSG_PRT_FMT "\n", MSG_FIELDS(*udp_data));
                    pkt_offset += udp_hdr->dgram_len;
                    printf("pkt_offset = %u\n", pkt_offset);
                    break;
                }
                default: {
                    printf("IPv4 protocol %u not interpreted\n", ip_hdr->next_proto_id);
                    uint8_t *raw_data = (uint8_t *)ip_hdr;
                    for (int j = 0; j < sizeof(rte_ipv4_hdr); j+=16) {
                        for (int k = 0; k < 16 && k+j < sizeof(rte_ipv4_hdr); k++) {
                            printf("%02x ", raw_data[j+k]);
                        }
                        printf("\n");
                    }
                }
                } // End: switch(ip_hdr->next_proto_id)
                break;
            }
            default:
                printf("Ethertype %04x not interpreted\n", ether_type);
            } // End: switch(ether_type)

            // Print raw data ------------------------------------------------//
            printf("(%u bytes read)\n", pkt_offset);
            // printf("# Raw data\n");
            // uint8_t *data = rte_pktmbuf_mtod(pkt, uint8_t *);
            // int j, k;
            // for (j = 0; j < pkt->pkt_len && j < 64; j += 16) {
            //     for (k = 0; k < 16 && k+j < pkt->pkt_len && k+j < 64; k++) {
            //         printf("%02x ", (data[j+k]));
            //     }
            //     printf("\n");
            // }
            // if (j+k < pkt->pkt_len - 16) {
            //     printf("...\n");
            //     for (j = pkt->pkt_len - 16; j < pkt->pkt_len; j++) {
            //         printf("%02x ", data[j]);
            //     }
            //     printf("\n");
            // }



            // Free packet when done
            rte_pktmbuf_free(bufs[i]);
        }
        
        const uint16_t nb_tx = rte_eth_tx_burst(port, 0, bufs, nb_tx);
        // Manually free any remaining packets. TX will free all other
        if (unlikely(nb_tx < nb_rx)) {
            uint16_t buf;
            for (buf = nb_tx; buf < nb_rx; buf++) {
                rte_pktmbuf_free(bufs[buf]);
            }
        }
        
    }
}

__rte_noreturn int lcore_fwrd_main(void *args)
{
    printf("Forwarding on lcore %u\n", rte_lcore_id());
    
    lcore_fwrd(((receiver_args_t *)args)->rx_counter);

    for (;;) {}
}


/**
 * Example application
 * 
 * Forward packets to a paired port
 */
static __rte_noreturn void lcore_fwd(__rte_unused void *arg)
{
    uint16_t port;

    // Check that the port is on the same NUMA node as the polling thread
    // for best performance
    RTE_ETH_FOREACH_DEV(port) {
        if (rte_eth_dev_socket_id(port) >= 0 && 
            rte_eth_dev_socket_id(port) != (int)rte_socket_id()) 
        {
            printf("WARNING, port %u is on remote NUMA node to"
                   "polling thread.\n\tPerformance will not be optimal.\n",port);
        }
    }

    printf("\nCore %u forwarding packets. [Ctrl+C to quit]");

    // Main work of application loop
    for (;;) {
        /// Receive packets on a port
        RTE_ETH_FOREACH_DEV(port) {
            // Get burst of RX packets, from first port of pair
            struct rte_mbuf *bufs[BURST_SIZE];
            const uint16_t nb_rx = rte_eth_rx_burst(port, 0, bufs, BURST_SIZE);

            if (unlikely(nb_rx == 0)) continue;

            // Send burst of TX packets, to second port of pair
            const uint16_t nb_tx = rte_eth_tx_burst(port, 0, bufs, nb_tx);

            // Manually free any remaining packets. TX will free all other
            if (unlikely(nb_tx < nb_rx)) {
                uint16_t buf;
                for (buf = nb_tx; buf < nb_rx; buf++) {
                    rte_pktmbuf_free(bufs[buf]);
                }
            }
        }
    }
}

