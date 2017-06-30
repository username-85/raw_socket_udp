#include "util.h"
#include <stdio.h>
#include <stdlib.h>

void err_sys_exit(char *msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}

void err_exit(char *msg)
{
	fprintf(stderr, msg);
	exit(EXIT_FAILURE);
}

// Function for checksum calculation. From the RFC,
unsigned short calc_csum(unsigned short *addr, int len)
{
	long sum = 0;

	while (len > 1) {
		sum += *(addr++);
		len -= 2;
	}

	if (len > 0)
		sum += *addr;

	while (sum >> 16)
		sum = ((sum & 0xffff) + (sum >> 16));

	sum = ~sum;

	return ((u_short) sum);
}

