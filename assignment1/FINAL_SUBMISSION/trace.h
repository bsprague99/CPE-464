#include "checksum.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include "checksum.h"
#include <arpa/inet.h>
#include <pcap/pcap.h>
#include <sys/types.h>
#include <netinet/ether.h>          /* LINUX */
/* #include <net/ethernet.h> */     /* MAC OS */

#define ARP  0x0806
#define IP   0x0800
#define UDP  0x11
#define TCP  0x06
#define ICMP 0x01
#define REPLY 0x0 
#define REQUEST 0x08
#define bad_cksum_ICMP 0x6d

uint8_t ip_length(const uint8_t *pkt_data);
void ether_info(const uint8_t *pkt_data, int in_byte_count, int packet_num);
uint16_t ether_next(const uint8_t *pkt_data);
uint8_t ip_next(const uint8_t *pkt_data);
void tcp_flag(int ip_len, const uint8_t *pkt_data);
void tcp_info(uint16_t tcp_len, uint8_t chksum_buff[1512], int ip_len, const uint8_t *pkt_data);
void udp_info(int len, const uint8_t *pkt_data);
void icmp_info(int ip_len, const uint8_t *pkt_data);
void ip_info(const uint8_t *pkt_data);
void arp_info(const uint8_t *pkt_data);
