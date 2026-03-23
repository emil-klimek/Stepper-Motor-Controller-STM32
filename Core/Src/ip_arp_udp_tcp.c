


#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "net.h"
#include "enc28j60.h"
#include "ip_arp_udp_tcp.h"

static uint8_t wwwport_l=80; // server port
static uint8_t wwwport_h=0;  // Note: never use same as TCPCLIENT_SRC_PORT_H

void (*icmp_callback)(uint8_t *ip);
// 0=wait, 1=first req no anser, 2=have gwmac, 4=refeshing but have gw mac, 8=accept an arp reply

#define WGW_INITIAL_ARP 1
#define WGW_HAVE_GW_MAC 2
#define WGW_REFRESHING 4
#define WGW_ACCEPT_ARP_REPLY 8

static int16_t delaycnt=0;
static uint8_t gwip[4];
static uint8_t gwmacaddr[6];
static uint8_t tcpsrvip[4];
volatile uint8_t waitgwmac=WGW_INITIAL_ARP;
uint8_t macaddr[6];
uint8_t ipaddr[4];
static uint16_t info_data_len=0;

uint8_t seqnum=0xa; // my initial tcp sequence number

#define CLIENTMSS 550
#define TCP_DATA_START ((uint16_t)TCP_SRC_PORT_H_P+(buf[TCP_HEADER_LEN_P]>>4)*4)

static const char arpreqhdr[] ={0,1,8,0,6,4,0,1};

static const char iphdr[] ={0x45,0,0,0x82,0,0,0x40,0,0x20}; // 0x82 is the total len on ip, 0x20 is ttl (time to live)


// is it an arp reply (no len check here, you must first call eth_type_is_arp_and_my_ip)
uint8_t eth_type_is_arp_reply(uint8_t *buf){
  return (buf[ETH_ARP_OPCODE_L_P]==ETH_ARP_OPCODE_REPLY_L_V);
}

// is it an arp request (no len check here, you must first call eth_type_is_arp_and_my_ip)
uint8_t eth_type_is_arp_req(uint8_t *buf){
  return (buf[ETH_ARP_OPCODE_L_P]==ETH_ARP_OPCODE_REQ_L_V);
}

uint16_t get_udp_data_len(uint8_t *buf)
{
	int16_t i;
	i=(((int16_t)buf[IP_TOTLEN_H_P])<<8)|(buf[IP_TOTLEN_L_P]&0xff);
	i-=IP_HEADER_LEN;
	i-=8;
	if (i<=0){
		i=0;
	}
	return((uint16_t)i);
}

uint16_t checksum(uint8_t *buf, uint16_t len,uint8_t type)
{
  // type 0=ip , icmp
  //      1=udp
  //      2=tcp
  uint32_t sum = 0;

  //if(type==0){
  //        // do not add anything, standard IP checksum as described above
  //        // Usable for ICMP and IP header
  //}
  if(type==1){
    sum+=IP_PROTO_UDP_V; // protocol udp
    // the length here is the length of udp (data+header len)
    // =length given to this function - (IP.scr+IP.dst length)
    sum+=len-8; // = real udp len
  }
  if(type==2){
    sum+=IP_PROTO_TCP_V;
    // the length here is the length of tcp (data+header len)
    // =length given to this function - (IP.scr+IP.dst length)
    sum+=len-8; // = real tcp len
  }
  // build the sum of 16bit words
  while(len >1){
    sum += 0xFFFF & (((uint32_t)*buf<<8)|*(buf+1));
    buf+=2;
    len-=2;
  }
  // if there is a byte left then add it (padded with zero)
  if (len){
    sum += ((uint32_t)(0xFF & *buf))<<8;
  }
  // now calculate the sum over the bytes in the sum
  // until the result is only 16bit long
  while (sum>>16){
    sum = (sum & 0xFFFF)+(sum >> 16);
  }
  // build 1's complement:
  return( (uint16_t) sum ^ 0xFFFF);
}


void init_ip_arp_udp_tcp(uint8_t *mymac,uint8_t *myip,uint16_t port)
{
  wwwport_h=(port>>8)&0xff;
  wwwport_l=(port&0xff);
  memcpy(ipaddr, myip, 4);
  memcpy(macaddr, mymac, 6);
}

uint8_t check_ip_message_is_from(uint8_t *buf,uint8_t *ip)
{
  return !memcmp(&buf[IP_SRC_P], ip, 4);
}

uint8_t eth_type_is_arp_and_my_ip(uint8_t *buf,uint16_t len)
{
  //
  if (len<41){
          return(0);
  }
  if(buf[ETH_TYPE_H_P] != ETHTYPE_ARP_H_V ||
      buf[ETH_TYPE_L_P] != ETHTYPE_ARP_L_V){
          return(0);
  }

  if (memcmp(&buf[ETH_ARP_DST_IP_P], ipaddr, 4)) {
    return 0;
  }

  return(1);
}

uint8_t eth_type_is_ip_and_my_ip(uint8_t *buf,uint16_t len){
  //eth+ip+udp header is 42
  if (len<42){
          return(0);
  }
  if(buf[ETH_TYPE_H_P]!=ETHTYPE_IP_H_V ||
      buf[ETH_TYPE_L_P]!=ETHTYPE_IP_L_V){
          return(0);
  }
  if (buf[IP_HEADER_LEN_VER_P]!=0x45){
          // must be IP V4 and 20 byte header
          return(0);
  }
  if (memcmp(&buf[IP_DST_P], ipaddr, 4)) {
    return 0;
  }
  return(1);
}

// make a return eth header from a received eth packet
void make_eth(uint8_t *buf)
{
  //copy the destination mac from the source and fill my mac into src
  memcpy(&buf[ETH_DST_MAC], &buf[ETH_SRC_MAC], 6);
  memcpy(&buf[ETH_SRC_MAC], macaddr, 6);
}

// make a new eth header for IP packet
void make_eth_ip_new(uint8_t *buf, uint8_t* dst_mac)
{
  //copy the destination mac from the source and fill my mac into src
  memcpy(&buf[ETH_DST_MAC], dst_mac, 6);
  memcpy(&buf[ETH_SRC_MAC], macaddr, 6);

  buf[ETH_TYPE_H_P] = ETHTYPE_IP_H_V;
  buf[ETH_TYPE_L_P] = ETHTYPE_IP_L_V;
}

void fill_ip_hdr_checksum(uint8_t *buf)
{
  uint16_t ck;
  // clear the 2 byte checksum
  buf[IP_CHECKSUM_P]=0;
  buf[IP_CHECKSUM_P+1]=0;
  buf[IP_FLAGS_P]=0x40; // don't fragment
  buf[IP_FLAGS_P+1]=0;  // fragement offset
  buf[IP_TTL_P]=64; // ttl
  // calculate the checksum:
  ck=checksum(&buf[IP_P], IP_HEADER_LEN,0);
  buf[IP_CHECKSUM_P]=ck>>8;
  buf[IP_CHECKSUM_P+1]=ck & 0xff;
}

void make_ip(uint8_t *buf)
{
  memcpy(&buf[IP_DST_P], &buf[IP_SRC_P], 4);
  memcpy(&buf[IP_SRC_P], ipaddr, 4);
  fill_ip_hdr_checksum(buf);
}

void make_arp_answer_from_request(uint8_t *buf)
{
  //
  make_eth(buf);
  buf[ETH_ARP_OPCODE_H_P]=ETH_ARP_OPCODE_REPLY_H_V;
  buf[ETH_ARP_OPCODE_L_P]=ETH_ARP_OPCODE_REPLY_L_V;
  // fill the mac addresses:
  memcpy(&buf[ETH_ARP_DST_MAC_P], &buf[ETH_ARP_SRC_MAC_P], 6);
  memcpy(&buf[ETH_ARP_SRC_MAC_P], macaddr, 6);
  // fill the ip addresses
  memcpy(&buf[ETH_ARP_DST_IP_P], &buf[ETH_ARP_SRC_IP_P], 4);
  memcpy(&buf[ETH_ARP_SRC_IP_P], ipaddr, 4);
  // eth+arp is 42 bytes:
  enc28j60PacketSend(42,buf);
}

// swap seq and ack number and count ack number up
void step_seq(uint8_t *buf,uint16_t rel_ack_num,uint8_t cp_seq)
{
  uint8_t i;
  uint8_t tseq;
  i=4;
  // sequence numbers:
  // add the rel ack num to SEQACK
  while(i>0) {
    rel_ack_num=buf[TCP_SEQ_H_P+i-1]+rel_ack_num;
    tseq=buf[TCP_SEQACK_H_P+i-1];
    buf[TCP_SEQACK_H_P+i-1]=0xff&rel_ack_num;
    if (cp_seq){
            // copy the acknum sent to us into the sequence number
            buf[TCP_SEQ_H_P+i-1]=tseq;
    }else{
            buf[TCP_SEQ_H_P+i-1]= 0; // some preset vallue
    }
    rel_ack_num=rel_ack_num>>8;
    i--;
  }
}

void make_echo_reply_from_request(uint8_t *buf,uint16_t len)
{
  make_eth(buf);
  make_ip(buf);
  buf[ICMP_TYPE_P]=ICMP_TYPE_ECHOREPLY_V;
  // we changed only the icmp.type field from request(=8) to reply(=0).
  // we can therefore easily correct the checksum:
  if (buf[ICMP_CHECKSUM_P] > (0xff-0x08)){
          buf[ICMP_CHECKSUM_P+1]++;
  }
  buf[ICMP_CHECKSUM_P]+=0x08;
  //
  enc28j60PacketSend(len,buf);
}

// you can send a max of 220 bytes of data
void make_udp_reply_from_request(uint8_t *buf,char *data,uint16_t datalen,uint16_t port)
{
  uint16_t ck;
  make_eth(buf);
  if (datalen>220){
    datalen=220;
  }
  // total length field in the IP header must be set:
  buf[IP_TOTLEN_H_P]=(IP_HEADER_LEN+UDP_HEADER_LEN+datalen) >>8;
  buf[IP_TOTLEN_L_P]=(IP_HEADER_LEN+UDP_HEADER_LEN+datalen) & 0xff;
  make_ip(buf);
  // send to port:
  //buf[UDP_DST_PORT_H_P]=port>>8;
  //buf[UDP_DST_PORT_L_P]=port & 0xff;
  // sent to port of sender and use "port" as own source:
  buf[UDP_DST_PORT_H_P]=buf[UDP_SRC_PORT_H_P];
  buf[UDP_DST_PORT_L_P]= buf[UDP_SRC_PORT_L_P];
  buf[UDP_SRC_PORT_H_P]=port>>8;
  buf[UDP_SRC_PORT_L_P]=port & 0xff;
  // calculte the udp length:
  buf[UDP_LEN_H_P]=(UDP_HEADER_LEN+datalen) >> 8;
  buf[UDP_LEN_L_P]=(UDP_HEADER_LEN+datalen) & 0xff;
  // zero the checksum
  buf[UDP_CHECKSUM_H_P]=0;
  buf[UDP_CHECKSUM_L_P]=0;
  // copy the data:
  memcpy(&buf[UDP_DATA_P], data, datalen);

  ck=checksum(&buf[IP_SRC_P], 16 + datalen,1);
  buf[UDP_CHECKSUM_H_P]=ck>>8;
  buf[UDP_CHECKSUM_L_P]=ck & 0xff;
  enc28j60PacketSend(UDP_HEADER_LEN+IP_HEADER_LEN+ETH_HEADER_LEN+datalen,buf);
}

// this is for the server not the client:
void make_tcp_synack_from_syn(uint8_t *buf)
{
  uint16_t ck;
  make_eth(buf);
  // total length field in the IP header must be set:
  // 20 bytes IP + 24 bytes (20tcp+4tcp options)
  buf[IP_TOTLEN_H_P]=0;
  buf[IP_TOTLEN_L_P]=IP_HEADER_LEN+TCP_HEADER_LEN_PLAIN+4;
  make_ip(buf);
  buf[TCP_FLAGS_P]=TCP_FLAGS_SYNACK_V;
  make_tcphead(buf,1,0);
  // put an inital seq number
  buf[TCP_SEQ_H_P+0]= 0;
  buf[TCP_SEQ_H_P+1]= 0;
  // we step only the second byte, this allows us to send packts
  // with 255 bytes, 512  or 765 (step by 3) without generating
  // overlapping numbers.
  buf[TCP_SEQ_H_P+2]= seqnum;
  buf[TCP_SEQ_H_P+3]= 0;
  // step the inititial seq num by something we will not use
  // during this tcp session:
  seqnum+=3;
  // add an mss options field with MSS to 1280:
  // 1280 in hex is 0x500
  buf[TCP_OPTIONS_P]=2;
  buf[TCP_OPTIONS_P+1]=4;
  buf[TCP_OPTIONS_P+2]=0x05;
  buf[TCP_OPTIONS_P+3]=0x0;
  // The tcp header length is only a 4 bit field (the upper 4 bits).
  // It is calculated in units of 4 bytes.
  // E.g 24 bytes: 24/4=6 => 0x60=header len field
  buf[TCP_HEADER_LEN_P]=0x60;
  // here we must just be sure that the web browser contacting us
  // will send only one get packet
  buf[TCP_WIN_SIZE]=0x5; // 1400=0x578
  buf[TCP_WIN_SIZE+1]=0x78;
  // calculate the checksum, len=8 (start from ip.src) + TCP_HEADER_LEN_PLAIN + 4 (one option: mss)
  ck=checksum(&buf[IP_SRC_P], 8+TCP_HEADER_LEN_PLAIN+4,2);
  buf[TCP_CHECKSUM_H_P]=ck>>8;
  buf[TCP_CHECKSUM_L_P]=ck& 0xff;
  // add 4 for option mss:
  enc28j60PacketSend(IP_HEADER_LEN+TCP_HEADER_LEN_PLAIN+4+ETH_HEADER_LEN,buf);
}

void fill_buf_p(uint8_t *buf,uint16_t len, const char *s)
{
  // fill in tcp data at position pos
  //
  // with no options the data starts after the checksum + 2 more bytes (urgent ptr)
  memcpy(buf, s, len);

}

void client_icmp_request(uint8_t *buf,uint8_t *destip)
{
  uint16_t ck;
  //
  memcpy(&buf[ETH_DST_MAC], gwmacaddr, 6); // gw mac in local lan or host mac
  memcpy(&buf[ETH_SRC_MAC], macaddr, 6);
  buf[ETH_TYPE_H_P] = ETHTYPE_IP_H_V;
  buf[ETH_TYPE_L_P] = ETHTYPE_IP_L_V;
  fill_buf_p(&buf[IP_P],9,iphdr);

  buf[IP_TOTLEN_L_P]=0x82;        // TUX Code has 0x54, here has 0x82
  buf[IP_PROTO_P]=IP_PROTO_UDP_V;
  memcpy(&buf[IP_DST_P], destip, 4);
  memcpy(&buf[IP_SRC_P], ipaddr, 4);
  fill_ip_hdr_checksum(buf);

  buf[ICMP_TYPE_P]=ICMP_TYPE_ECHOREQUEST_V;
  buf[ICMP_TYPE_P+1]=0; // code
  // zero the checksum
  buf[ICMP_CHECKSUM_H_P]=0;
  buf[ICMP_CHECKSUM_L_P]=0;
  // a possibly unique id of this host:
  buf[ICMP_IDENT_H_P]=5; // some number
  buf[ICMP_IDENT_L_P]=ipaddr[3]; // last byte of my IP
  //
  buf[ICMP_IDENT_L_P+1]=0; // seq number, high byte
  buf[ICMP_IDENT_L_P+2]=1; // seq number, low byte, we send only 1 ping at a time
  // copy the data:
  memset(&buf[ICMP_DATA_P], PINGPATTERN, 56);
  //
  ck=checksum(&buf[ICMP_TYPE_P], 56+8,0);
  buf[ICMP_CHECKSUM_H_P]=ck>>8;
  buf[ICMP_CHECKSUM_L_P]=ck& 0xff;
  enc28j60PacketSend(98,buf);
}

void send_udp_prepare(uint8_t *buf,uint16_t sport, uint8_t *dip, uint16_t dport)
{
  memcpy(&buf[ETH_DST_MAC], gwmacaddr, 6);
  memcpy(&buf[ETH_SRC_MAC], macaddr, 6);
  buf[ETH_TYPE_H_P] = ETHTYPE_IP_H_V;
  buf[ETH_TYPE_L_P] = ETHTYPE_IP_L_V;
  fill_buf_p(&buf[IP_P],9,iphdr);

  // total length field in the IP header must be set:
  buf[IP_TOTLEN_H_P]=0;
  // done in transmit: buf[IP_TOTLEN_L_P]=IP_HEADER_LEN+UDP_HEADER_LEN+datalen;
  buf[IP_PROTO_P]=IP_PROTO_UDP_V;
  memcpy(&buf[IP_DST_P], dip, 4);
  memcpy(&buf[IP_SRC_P], ipaddr, 4);

  // done in transmit: fill_ip_hdr_checksum(buf);
  buf[UDP_DST_PORT_H_P]=(dport>>8);
  buf[UDP_DST_PORT_L_P]=0xff&dport;
  buf[UDP_SRC_PORT_H_P]=(sport>>8);
  buf[UDP_SRC_PORT_L_P]=sport&0xff;
  buf[UDP_LEN_H_P]=0;
  // done in transmit: buf[UDP_LEN_L_P]=UDP_HEADER_LEN+datalen;
  // zero the checksum
  buf[UDP_CHECKSUM_H_P]=0;
  buf[UDP_CHECKSUM_L_P]=0;
  // copy the data:
  // now starting with the first byte at buf[UDP_DATA_P]
}

void send_udp_transmit(uint8_t *buf,uint16_t datalen)
{
  uint16_t ck;
  buf[IP_TOTLEN_H_P]=(IP_HEADER_LEN+UDP_HEADER_LEN+datalen) >> 8;
  buf[IP_TOTLEN_L_P]=(IP_HEADER_LEN+UDP_HEADER_LEN+datalen) & 0xff;
  fill_ip_hdr_checksum(buf);
  //buf[UDP_LEN_L_P]=UDP_HEADER_LEN+datalen;
  buf[UDP_LEN_H_P]=(UDP_HEADER_LEN+datalen) >>8;
  buf[UDP_LEN_L_P]=(UDP_HEADER_LEN+datalen) & 0xff;

  //
  ck=checksum(&buf[IP_SRC_P], 16 + datalen,1);
  buf[UDP_CHECKSUM_H_P]=ck>>8;
  buf[UDP_CHECKSUM_L_P]=ck& 0xff;
  enc28j60PacketSend(UDP_HEADER_LEN+IP_HEADER_LEN+ETH_HEADER_LEN+datalen,buf);
}

void send_udp(uint8_t *buf,char *data,uint16_t datalen,uint16_t sport, uint8_t *dip, uint16_t dport)
{
  send_udp_prepare(buf,sport, dip, dport);
  // limit the length: Why??? ADL
  // NOTE: We dont need such silly limits on powerful STM32s
  // if (datalen>220){
  //         datalen=220;
  // }
  // copy the data:
  memcpy(&buf[UDP_DATA_P], data, datalen);
  //
  send_udp_transmit(buf,datalen);
}

void client_arp_whohas(uint8_t *buf,uint8_t *ip_we_search)
{
  //
  memset(&buf[ETH_DST_MAC], 0xFF, 6);
  memcpy(&buf[ETH_SRC_MAC], macaddr, 6);
  buf[ETH_TYPE_H_P] = ETHTYPE_ARP_H_V;
  buf[ETH_TYPE_L_P] = ETHTYPE_ARP_L_V;
  fill_buf_p(&buf[ETH_ARP_P],8,arpreqhdr);

  memcpy(&buf[ETH_ARP_SRC_MAC_P], macaddr, 6);
  memset(&buf[ETH_ARP_DST_MAC_P], 0, 6);

  memcpy(&buf[ETH_ARP_DST_IP_P], ip_we_search, 4);
  memcpy(&buf[ETH_ARP_SRC_IP_P], ipaddr, 4);

  waitgwmac|=WGW_ACCEPT_ARP_REPLY;

  // 0x2a=42=len of packet
  enc28j60PacketSend(0x2a,buf);
}

uint8_t client_waiting_gw(void)
{
  if (waitgwmac & WGW_HAVE_GW_MAC){
    return(0);
  }
  return(1);
}

// store the mac addr from an arp reply
// no len check here, you must first call eth_type_is_arp_and_my_ip
uint8_t client_store_gw_mac(uint8_t *buf)
{
  if (memcmp(&buf[ETH_ARP_SRC_IP_P], gwip, 4)) {
    return 0;
  }

  memcpy(gwmacaddr, &buf[ETH_ARP_SRC_MAC_P], 6);
  return 1;
}

void client_gw_arp_refresh(void) {
  if (waitgwmac & WGW_HAVE_GW_MAC){
    waitgwmac|=WGW_REFRESHING;
  }
}

void client_set_gwip(uint8_t *gwipaddr)
{
  waitgwmac=WGW_INITIAL_ARP; // causes an arp request in the packet loop
  memcpy(gwip, gwipaddr, 4);
}

void client_tcp_set_serverip(uint8_t *ipaddr)
{
  memcpy(tcpsrvip, ipaddr, 4);
}

void register_ping_rec_callback(void (*callback)(uint8_t *srcip))
{
  icmp_callback=callback;
}

// loop over this to check if we get a ping reply:
uint8_t packetloop_icmp_checkreply(uint8_t *buf,uint8_t *ip_monitoredhost)
{
  if(buf[IP_PROTO_P]==IP_PROTO_ICMP_V && buf[ICMP_TYPE_P]==ICMP_TYPE_ECHOREPLY_V){
    if (buf[ICMP_DATA_P]== PINGPATTERN){
      if (check_ip_message_is_from(buf,ip_monitoredhost)){
              return(1);
              // ping reply is from monitored host and ping was from us
      }
    }
  }
  return(0);
}

uint16_t if_plen_eq_0(uint8_t * buf, uint16_t plen)
{
	if (plen == 0) {
	    if ((waitgwmac & WGW_INITIAL_ARP || waitgwmac & WGW_REFRESHING) && delaycnt == 0 && enc28j60linkup()) {
	      client_arp_whohas(buf, gwip);
	    }
	    delaycnt++;


	    return (0);
	  }
}

uint16_t if_eth_type_is_arp_and_my_ip(uint8_t * buf, uint16_t plen)
{
    if (buf[ETH_ARP_OPCODE_L_P] == ETH_ARP_OPCODE_REQ_L_V) {
      // is it an arp request
      make_arp_answer_from_request(buf);
    }

    if (waitgwmac & WGW_ACCEPT_ARP_REPLY && (buf[ETH_ARP_OPCODE_L_P] == ETH_ARP_OPCODE_REPLY_L_V)) {
      // is it an arp reply
      if (client_store_gw_mac(buf)) {
        waitgwmac = WGW_HAVE_GW_MAC;
      }
    }

    return (0);
}


uint16_t if_proto_icmp_and_icmp_type_echorequest_b(uint8_t * buf, uint16_t plen)
{
    if (icmp_callback) {
      ( * icmp_callback)( & (buf[IP_SRC_P]));
    }
    // a ping packet, let's send pong
    make_echo_reply_from_request(buf, plen);
    ES_PingCallback();
    return (0);
}

uint16_t packetloop_icmp_tcp(uint8_t * buf, uint16_t plen) {

  //plen will be unequal to zero if there is a valid
  // packet (without crc error):

  if (plen == 0)
	  return if_plen_eq_0(buf, plen);

  if (eth_type_is_arp_and_my_ip(buf, plen))
	  return if_eth_type_is_arp_and_my_ip(buf,plen);

  if (eth_type_is_ip_and_my_ip(buf, plen) == 0)
	  return 0;

  if (buf[IP_PROTO_P] == IP_PROTO_ICMP_V && buf[ICMP_TYPE_P] == ICMP_TYPE_ECHOREQUEST_V)
	  return if_proto_icmp_and_icmp_type_echorequest_b(buf,plen);

  if (plen < 54 && buf[IP_PROTO_P] != IP_PROTO_TCP_V)
	  return 0;

  return 0;
}
