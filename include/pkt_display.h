#pragma once

#include <rte_arp.h>
/**
 * Print ARP message content and return the size of content read. 
 */
int print_arp_hdr(rte_arp_hdr *arp_hdr);

#include <rte_ether.h>
/**
 * Print Ethernet header and return the size of content read 
 */
int print_eth_hdr(rte_ether_hdr *eth);

#include <rte_icmp.h>
/**
 * Print ICMP packet and return the size of content read
 */
int print_icmp_hdr(rte_icmp_hdr *icmp);

#include <rte_ip.h>
/**
 * Struct to easily convert IPv4 address between 32 bit and 4x8bit
 */
struct ip_addr_t {
    union {
        uint32_t as_int;
        uint8_t  as_addr[4];
    };
};
/**
 * Macro to print the 4 bytes of an IPv4 address
 */
#define IPV4_ADDR_PRT_FMT       "%u.%u.%u.%u"
/**
 * Macro to extract the IPv4 address bytes from ip_addr_t struct
 */
#define IPV4_ADDR_BYTES(addr)   ((addr).as_addr[0]), ((addr).as_addr[1]), \
                                ((addr).as_addr[2]), ((addr).as_addr[3])
/**
 * Print IPv4 header and return the size of content read
 */
int print_ipv4_hdr(rte_ipv4_hdr *ip_hdr);