#include <stdint.h>
#include <stdio.h>

#include "pkt_display.h"

int print_arp_hdr(rte_arp_hdr *arp_hdr)
{
    size_t start = (size_t)arp_hdr;
    printf("# ARP header\n");
    // Fields not marked as be, but seem to still be wrong endian
    printf("HTYPE = %x\n", rte_be_to_cpu_16(arp_hdr->arp_hardware));
    printf("PTYPE = %x\n", arp_hdr->arp_protocol);
    printf("HLEN  = %x\n", arp_hdr->arp_hlen);
    printf("PLEN  = %x\n", arp_hdr->arp_plen);
    printf("OPER  = %x\n", rte_be_to_cpu_16(arp_hdr->arp_opcode));
    rte_arp_ipv4 *arp_pkt;
    // Get ARP content after header
    arp_pkt = (rte_arp_ipv4 *)(&(arp_hdr[1]));
    printf("# ARP packet\n");
    printf("SHA   = %x\n", arp_pkt->arp_sha);
    printf("SPA   = %x\n", arp_pkt->arp_sip);
    printf("THA   = %x\n", arp_pkt->arp_tha);
    printf("TPA   = %x\n", arp_pkt->arp_tip);

    return sizeof(rte_arp_hdr) + sizeof(rte_arp_ipv4);
}

int print_eth_hdr(rte_ether_hdr *eth)
{
    printf("# Ethernet header (%u bytes)\n", sizeof(rte_ether_hdr));
    printf("eth_type = 0x%04x\n", rte_be_to_cpu_16(eth->ether_type));
    printf("dst_addr (MAC) = " RTE_ETHER_ADDR_PRT_FMT "\n", 
            RTE_ETHER_ADDR_BYTES(&(eth->dst_addr)));
    printf("dst_addr (MAC) = " RTE_ETHER_ADDR_PRT_FMT "\n", 
            RTE_ETHER_ADDR_BYTES(&(eth->src_addr)));
    
    return sizeof(rte_ether_hdr);
}

int print_icmp_hdr(rte_icmp_hdr *icmp)
{
    printf("# ICMP\n");
    printf("type   = 0x%02x\n", icmp->icmp_type);
    printf("code   = 0x%02x\n", icmp->icmp_code);
    printf("cksum  = 0x%04x\n", rte_be_to_cpu_16(icmp->icmp_cksum));
    printf("ident  =   %u\n", rte_be_to_cpu_16(icmp->icmp_ident));
    printf("seq_nb =   %u\n", rte_be_to_cpu_16(icmp->icmp_seq_nb));

    return sizeof(rte_icmp_hdr);
}

int print_ipv4_hdr(rte_ipv4_hdr *ip_hdr)
{
    ip_addr_t ip_addr;
    
    printf("# IPv4 header (%u)\n", sizeof(rte_ether_hdr));
    ip_addr.as_int = (ip_hdr->dst_addr);
    printf("ip_dst_addr = " IPV4_ADDR_PRT_FMT "\n", IPV4_ADDR_BYTES(ip_addr));
    ip_addr.as_int = (ip_hdr->src_addr);
    printf("ip_src_addr = " IPV4_ADDR_PRT_FMT "\n", IPV4_ADDR_BYTES(ip_addr));
    printf("ttl         = %u\n", ip_hdr->time_to_live);
    printf("total_len   = %u\n", (ip_hdr->total_length));
    printf("version/ihl = 0x%02x (v: %u, ihl: %u)\n", 
    ip_hdr->version_ihl, ip_hdr->version, ip_hdr->ihl);
    printf("protocol    = 0x%x (%u)\n", 
        ip_hdr->next_proto_id, ip_hdr->next_proto_id);

    return sizeof(rte_ipv4_hdr);
}
