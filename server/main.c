#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PORT_SRV "9877"
#define BACKLOG 10
#define MAXDSIZE 100

int main(void)
{
	socklen_t addrlen = 0;
	int sockfd = inet_bind(PORT_SRV, SOCK_DGRAM, &addrlen);
	if (sockfd < 0)
		err_exit("could not create socket\n");

	printf("waiting for messages ...\n");
	struct sockaddr_storage client_addr;
	socklen_t ca_size = sizeof(client_addr);
	while(1) {
		char msg[MAXDSIZE] = {0};
		int numbytes = recvfrom(sockfd, msg, MAXDSIZE - 1, 0,
		                        (struct sockaddr *)&client_addr,
		                        &ca_size);
		if (numbytes == -1)
			err_sys_exit("recvfrom");

		msg[numbytes] = '\0';

		char addr_str[INET6_ADDRSTRLEN + 10] = {0};
		inet_addr_str((struct sockaddr *)&client_addr,
		              ca_size, addr_str, sizeof(addr_str));
		printf("message '%s' from %s\n", msg, addr_str);

		numbytes = sendto(sockfd, msg, strlen(msg), 0,
		                  (struct sockaddr *)&client_addr, ca_size);
		if (numbytes == -1)
			err_sys_exit("sendto");
	}

	exit(EXIT_SUCCESS);
}

