#ifndef IP_UDP_H
#define IP_UDP_H

#include <controller.h>

C_API uint16_t get_udp_data_len(uint8_t *buf);
C_API uint16_t packetloop_icmp_udp(uint8_t *buf,uint16_t plen);
C_API uint16_t udp_packet_dport(unsigned char* buf);

#endif
