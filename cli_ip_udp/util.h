#ifndef UTIL_H
#define UTIL_H

void err_sys_exit(char *msg);
void err_exit(char *msg);
unsigned short calc_csum(unsigned short *addr, int len);

#endif
