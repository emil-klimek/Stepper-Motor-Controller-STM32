
#include <stdint.h>
#include <EtherShield.h>



#define WGW_ACCEPT_ARP_REPLY 8
#define WGW_HAVE_GW_MAC 2

static uint16_t info_data_len = 0;

extern volatile uint8_t waitgwmac;
extern void (*icmp_callback)(uint8_t *ip);



uint16_t packetloop_icmp_udp(uint8_t *buf,uint16_t plen)
{
	if(eth_type_is_arp_and_my_ip(buf,plen)){
		//fputs("icmp packet\n\r", stdout);
		if (buf[ETH_ARP_OPCODE_L_P]==ETH_ARP_OPCODE_REQ_L_V){
			//fputs("sending arp request\n\r",stdout);
			// is it an arp request
			make_arp_answer_from_request(buf);
		}

		//if defined NTP_client  UDP_client  TCP_client  PING_client

		if(waitgwmac & WGW_ACCEPT_ARP_REPLY && (buf[ETH_ARP_OPCODE_L_P] == ETH_ARP_OPCODE_REPLY_L_V))
		{
			if (client_store_gw_mac(buf)) {
				waitgwmac = WGW_HAVE_GW_MAC;
			}
		}



		return(0);
	}
	// check if ip packets are for us:
	if(eth_type_is_ip_and_my_ip(buf,plen)==0){
		return(0);
	}

	if(buf[IP_PROTO_P]==IP_PROTO_ICMP_V && buf[ICMP_TYPE_P]==ICMP_TYPE_ECHOREQUEST_V){
		make_echo_reply_from_request(buf,plen);
		return(0);
	}

	if(buf[IP_PROTO_P] == IP_PROTO_ICMP_V && buf[ICMP_TYPE_P] == ICMP_TYPE_ECHOREQUEST_V)
	{
		if(icmp_callback)
		{
			(*icmp_callback)(&(buf[IP_SRC_P]));
		}

		// a ping packet, let's send pong
		make_echo_reply_from_request(buf, plen);
		ES_PingCallback();
		return 0;
	}

	if (buf[IP_PROTO_P]==IP_PROTO_UDP_V) {
		info_data_len=get_udp_data_len(buf);
		return(IP_HEADER_LEN+8+14);
	}

	return(0);
}

uint16_t udp_packet_dport(unsigned char* buf)
{
	uint16_t dport = buf[UDP_DST_PORT_H_P] << 8;
		     dport |= buf[UDP_DST_PORT_L_P];

	return dport;
}
