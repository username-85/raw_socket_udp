#include "util.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/udp.h>
#include <netinet/ip.h>

#define PACK_SIZE 1024
#define MAX_DSIZE 200
#define SRV_PORT 9877
#define CLI_PORT 9888

int make_rsock(void);
void set_iph(struct iphdr * iph, int tot_len, char *srv_ip);
void set_udph(struct udphdr * udph, int data_len, int cli_port, int srv_port);
static void print_msgs(int sock, int msg_num);

int main (int argc, char *argv[])
{
	char *usage_msg =
	    "usage: client server_ip \"message\"\n";
	if (argc != 3)
		err_exit(usage_msg);

	int cli_port = CLI_PORT;
	int srv_port = SRV_PORT;
	char *srv_ip = argv[1];
	char *msg    = argv[2];

	int rsock = make_rsock();
	if (rsock < 0)
		err_exit("socket error");

	char *datagram = calloc(PACK_SIZE, sizeof(char));
	if (!datagram)
		err_sys_exit("calloc error");

	char *data = datagram + sizeof(struct iphdr) + sizeof(struct udphdr);
	strcpy(data, msg);

	struct udphdr *udph = (struct udphdr *)(datagram + sizeof(struct iphdr));
	set_udph(udph, strlen(data), cli_port, srv_port);

	int ip_totlen = sizeof(struct iphdr) + sizeof(struct udphdr)
	                + strlen(data);
	struct iphdr* iph = (struct iphdr *)datagram;
	set_iph(iph, ip_totlen, srv_ip);

	struct sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(srv_port);
	sin.sin_addr.s_addr = inet_addr(srv_ip);

	int nbytes = sendto(rsock, datagram, iph->tot_len, 0,
	                    (struct sockaddr *)&sin, sizeof(sin));
	free(datagram);

	if (nbytes < 0)
		err_exit("sendto failed");
	else
		printf("packet send. length : %d \n", nbytes);

	int msgs_num = 2; // 1 for send +  1 for received
	print_msgs(rsock, msgs_num);

	exit(EXIT_SUCCESS);
}

int make_rsock(void)
{
	int rsock = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
	if (rsock < 0) {
		perror("socket error");
		return -1;
	}

	// socket expects us to provide IPv4 header.
	int on = 1;
	if (setsockopt(rsock, IPPROTO_IP, IP_HDRINCL, &on, sizeof (on)) < 0) {
		perror("setsockopt error");
		return - 1;
	}
	return rsock;
}

void set_iph(struct iphdr * iph, int tot_len, char *srv_ip)
{
	iph->ihl = sizeof(struct iphdr) / sizeof (uint32_t);
	iph->version = 4;
	iph->tos = 0;
	iph->tot_len = tot_len;
	iph->id = htonl(0); // id of packet
	iph->frag_off = 0;
	iph->ttl = 255;
	iph->protocol = IPPROTO_UDP;
	iph->check = calc_csum((unsigned short *)iph, sizeof(struct iphdr));
	iph->saddr = 0;
	iph->daddr = inet_addr(srv_ip);
}

void set_udph(struct udphdr * udph, int data_len, int cli_port, int srv_port)
{
	udph->source = htons(cli_port);
	udph->dest = htons(srv_port);
	udph->len = htons(sizeof(struct udphdr) + data_len);
	udph->check = 0;
}

static void print_msgs(int sock, int msgs_num)
{
	for (int i = 0; i < msgs_num; i++) {
		char buf[MAX_DSIZE] = {0};

		int nbytes = recv(sock, buf, MAX_DSIZE, 0);
		if (nbytes == -1)
			err_sys_exit("recv");
		buf[nbytes]='\0';

		printf("received %d bytes, message '%s'\n", nbytes,
		       buf+ sizeof(struct iphdr) + sizeof(struct udphdr));
	}
}

