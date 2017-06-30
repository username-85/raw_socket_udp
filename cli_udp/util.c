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

