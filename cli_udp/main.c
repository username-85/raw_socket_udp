#include "util.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/udp.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#define PACK_SIZE 1024
#define MAX_DSIZE 100
#define SRV_PORT 9877
#define CLI_PORT 9888

static void print_msgs(int sock, int msgs_num);

int main (int argc, char *argv[])
{
	char *usage_msg =
	    "usage: client server_ip \"message\"\n";
	if (argc != 3)
		err_exit(usage_msg);

	int cli_port = CLI_PORT;
	char *srv_ip = argv[1];
	int srv_port = SRV_PORT;
	char *msg    = argv[2];

	int rsock = socket (AF_INET, SOCK_RAW, IPPROTO_UDP);
	if (rsock < 0)
		err_sys_exit("socket error");

	char *datagram = calloc(PACK_SIZE, sizeof(char));
	char *data     = datagram +  sizeof(struct udphdr);

	strncpy(data, msg, MAX_DSIZE);

	struct udphdr *udph = (struct udphdr *)datagram;
	int tot_len  = sizeof(struct udphdr) + strlen(data);
	udph->source = htons(cli_port);
	udph->dest   = htons(srv_port);
	udph->len    = htons(tot_len);
	udph->check  = 0; // checksum

	struct sockaddr_in sin;
	sin.sin_family      = AF_INET;
	sin.sin_port        = htons(srv_port);
	sin.sin_addr.s_addr = inet_addr(srv_ip);

	int nbytes = sendto(rsock, datagram, tot_len, 0,
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

