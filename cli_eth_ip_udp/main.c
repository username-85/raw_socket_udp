#include "util.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/ether.h>
#include <linux/if_packet.h>

#define PBUF_SIZE 65536
#define MAX_DSIZE 100
#define SRV_PORT 9877
#define CLI_PORT 9888

#define DEST_MAC0 0x04
#define DEST_MAC1 0x7d
#define DEST_MAC2 0x7b
#define DEST_MAC3 0x5f
#define DEST_MAC4 0x13
#define DEST_MAC5 0x5f

int make_rsock(void);
int set_ethh(struct ethhdr *eth, int socket, char *interface);
void set_iph(struct iphdr * iph, int ip_tot_len, int socket, char *interface,
             char *srv_ip);
void set_udph(struct udphdr * udph, int data_len, int cli_port, int srv_port);
int set_saddr(struct sockaddr_ll *saddr, int socket, char *interface);
static void print_packets(int sock, int pnum);

int main(int argc, char *argv[])
{
	char *usage_msg = "usage: client ifname server_ip \"message\"\n";
	if (argc != 4)
		err_exit(usage_msg);

	int cli_port = CLI_PORT;
	int srv_port = SRV_PORT;
	char *ifname = argv[1];
	char *srv_ip = argv[2];
	char *msg    = argv[3];

	int rsock = make_rsock();
	if (rsock < 0)
		err_exit("socket error");

	char *datagram = calloc(PBUF_SIZE, sizeof(char));
	if (!datagram)
		err_sys_exit("calloc error\n");

	char *data = datagram + sizeof(struct ethhdr) + sizeof(struct iphdr)
	             + sizeof(struct udphdr);
	strncpy(data, msg, MAX_DSIZE - 1);

	struct ethhdr *eth = (struct ethhdr *)datagram;
	if (set_ethh(eth, rsock, ifname) < 0)
		err_exit("set_eth error\n");

	struct iphdr *iph = (struct iphdr *)(datagram + sizeof(struct ethhdr));
	int ip_tot_len = sizeof(struct iphdr) + sizeof(struct udphdr)
	                 + strlen(data);
	set_iph(iph,ip_tot_len, rsock, ifname, srv_ip);

	struct udphdr *udph = (struct udphdr *)(datagram
	                                        + sizeof(struct ethhdr) + sizeof(struct iphdr));
	set_udph(udph, strlen(data), cli_port, srv_port);

	struct sockaddr_ll socket_address = {0};
	if (set_saddr(&socket_address, rsock, ifname) < 0)
		err_exit("set_saddr error\n");

	int datagram_len = ip_tot_len + sizeof(struct ethhdr);
	int nbytes = sendto(rsock, datagram, datagram_len, 0,
	                    (struct sockaddr*)&socket_address,
	                    sizeof(struct sockaddr_ll));
	free(datagram);

	if (nbytes < 0)
		err_exit("sendto failed\n");
	else
		printf("packet send. length : %d \n", nbytes);

	int packets_num = 1;
	print_packets(rsock, packets_num);

	exit(EXIT_SUCCESS);
}

int make_rsock(void)
{
	int rsock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IP));
	if (rsock < 0)
		return -1;

	return rsock;
}

int set_ethh(struct ethhdr *eth, int socket, char *interface)
{
	struct ifreq if_mac = {0};
	strncpy(if_mac.ifr_name, interface, IFNAMSIZ - 1);
	if (ioctl(socket, SIOCGIFHWADDR, &if_mac) < 0)
		return -1;

	eth->h_source[0] = (unsigned char)(if_mac.ifr_hwaddr.sa_data[0]);
	eth->h_source[1] = (unsigned char)(if_mac.ifr_hwaddr.sa_data[1]);
	eth->h_source[2] = (unsigned char)(if_mac.ifr_hwaddr.sa_data[2]);
	eth->h_source[3] = (unsigned char)(if_mac.ifr_hwaddr.sa_data[3]);
	eth->h_source[4] = (unsigned char)(if_mac.ifr_hwaddr.sa_data[4]);
	eth->h_source[5] = (unsigned char)(if_mac.ifr_hwaddr.sa_data[5]);

	eth->h_dest[0] = DEST_MAC0;
	eth->h_dest[1] = DEST_MAC1;
	eth->h_dest[2] = DEST_MAC2;
	eth->h_dest[3] = DEST_MAC3;
	eth->h_dest[4] = DEST_MAC4;
	eth->h_dest[5] = DEST_MAC5;

	eth->h_proto = htons(ETH_P_IP);

	return 0;
}

void set_iph(struct iphdr * iph, int ip_tot_len, int socket, char *interface,
             char *srv_ip)
{
	struct ifreq if_ip = {0};
	strncpy(if_ip.ifr_name, interface, IFNAMSIZ-1);
	if (ioctl(socket, SIOCGIFADDR, &if_ip) < 0)
		err_sys_exit("ioctl SIOCGIFADDR");

	iph->ihl      = sizeof(struct iphdr) / sizeof (uint32_t);
	iph->version  = 4;
	iph->tos      = 0;
	iph->tot_len  = htons(ip_tot_len);
	iph->id       = htonl(0);
	iph->frag_off = 0;
	iph->ttl      = 255;
	iph->protocol = IPPROTO_UDP;
	iph->saddr    = inet_addr(
	                    inet_ntoa(
	                        ((struct sockaddr_in *)&if_ip.ifr_addr)->sin_addr
	                    ));
	iph->daddr = inet_addr(srv_ip);
	iph->check = 0;
	iph->check = calc_csum((unsigned short *)iph, sizeof(struct iphdr));
}

void set_udph(struct udphdr * udph, int data_len, int cli_port, int srv_port)
{
	udph->source = htons(cli_port);
	udph->dest   = htons(srv_port);
	udph->len    = htons(sizeof(struct udphdr) + data_len);
	udph->check  = 0;
}

int set_saddr(struct sockaddr_ll *saddr, int socket, char *interface)
{
	struct ifreq if_idx = {0};
	strncpy(if_idx.ifr_name, interface, IFNAMSIZ - 1);
	if (ioctl(socket, SIOCGIFINDEX, &if_idx) < 0)
		return -1;

	saddr->sll_protocol = htons(ETH_P_IP);
	saddr->sll_ifindex = if_idx.ifr_ifindex;
	saddr->sll_halen   = ETH_ALEN;
	saddr->sll_family  = PF_PACKET;
	saddr->sll_addr[0] = DEST_MAC0;
	saddr->sll_addr[1] = DEST_MAC1;
	saddr->sll_addr[2] = DEST_MAC2;
	saddr->sll_addr[3] = DEST_MAC3;
	saddr->sll_addr[4] = DEST_MAC4;
	saddr->sll_addr[5] = DEST_MAC5;

	return 0;
}

static void print_packets(int sock, int pnum)
{
	for (int i = 0; i < pnum; i++) {
		char buf[MAX_DSIZE] = {0};

		int nbytes = recv(sock, buf, MAX_DSIZE - 1, 0);
		if (nbytes == -1)
			err_sys_exit("recv");
		buf[nbytes]='\0';

		printf("received %d bytes, message '%s'\n", nbytes,
		       buf + sizeof(struct ethhdr) + sizeof(struct iphdr) 
		       + sizeof(struct udphdr));
	}
}

