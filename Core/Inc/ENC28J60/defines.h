



#define ETHERNET_LED_GPIO GPIOA
#define ETHERNET_LED_PIN GPIO_PIN_4
#define ETHERNET_CS_GPIO GPIOB
#define ETHERNET_CS_PIN GPIO_PIN_0
#define ETHERNET_CS_DELAY 1



#define ETHERSHIELD_DEBUG 1
#define UDP_client 1
//#define NTP_client 1
//#define WOL_client 1
//#define DNS_client 1
//#define DHCP_client 1

#define DHCP_HOSTNAME "stm32-enc28j60"
#define DHCP_HOSTNAME_LEN 14

#define PING_client 1
#define PINGPATTERN 0x42

//#define TCP_client 1
//#define WWW_client 1

#define WWW_USER_AGENT "stm32-enc28j60/1.0"
#define FROMDECODE_websrv_help 1
#define URLENCODE_websrv_help 1
