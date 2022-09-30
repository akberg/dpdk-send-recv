# README

Example program for a sender-receiver attempting to saturate the network link through a SmartNIC. To be added, a forwarder to run on the SmartNIC, doing some simple computation, like playing fizzbuzz. Implementing fizzbuzz to make the traffic slightly less boring than just an increasing number sequence, and to make up a filtering workload for the SmartNIC.

## TODO
* Get the application running on SmartNIC: Bluefield-2 and Intel
* Move runtime parameters to command line arguments
* Figure out network settings limiting the send rate to 5-7 Gbps

## ARP request
```
# mbuf
packet_type = 00000001
Stack:
L2: L2_ETHER
L3: L3_UNKNOWN
L4: L4_UNKNOWN
Ethernet header? yes
IPv4 header? no
UDP header? no
ICMP header? no
pkt_type = 1
pkt_len = 60
data_len = 60
# Ethernet frame
eth_type = 2054
dst_addr (MAC) = b8:ce:f6:61:9f:8a
src_addr (MAC) = 98:3:9b:79:9a:20
# IPv4 frame
ip_dst_addr = 15.6.0.0
ip_src_addr = 154.32.15.15
ttl = 152
ip_hdr_total_length = 2048
00 01 08 00 06 04 00 01 
98 03 9b 79 9a 20 0f 0f 
0f 06 00 00 00 00 00 00 
0f 0f 0f 07 00 00 00 00 
00 00 00 00 00 00 00 00 
00 00 00 00 00 00 00 00 
00 00 00 00 00 00 00 00 
00 00 00 00 00 00 00 00 
```

## Errors

Errors when setting the delay too low or lcore number too high
```
mlx5_common: Failed to modify SQ using DevX
mlx5_net: Cannot change the Tx SQ state to RESET Device or resource busy
mlx5_net: Cannot change the Tx SQ state to RESET Remote I/O error
```