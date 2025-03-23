#pragma once

#include <winsock2.h>

#define PACKAGE_BUGREPORT "-no big report-"
#define STDRCFILE "%USERPROFILE%\\.vde2\\vde_switch.rc"
#define ETH_ALEN 6
#define INIT_HASH_SIZE 128
#define DEFAULT_PRIORITY 0x8000
#define INIT_NUMPORTS 32
#define DEFAULT_COST 20000000 /* 1Mbit line */

#define HASH_TABLE_SIZE_ARG 0x100
#define MACADDR_ARG         0x101
#define PRIORITY_ARG        0x102

#define NUMOFVLAN 4095
#define NOVLAN 0xfff

#define EXIST_OK 0
#define X_OK 01
#define W_OK 02
#define WX_OK 03
#define R_OK 04
#define RX_OK 05
#define RW_OK 06
#define RWX_OK 07

#define PRIOFLAG 0x80
#define TYPEMASK 0x7f
#define ISPRIO(X) ((X) & PRIOFLAG)

extern unsigned char switchmac[];
extern const char* prog;
extern char errorbuff[1024];

void* mainloop_get_private_data(SOCKET fd);
void mainloop_set_private_data(SOCKET fd, void* private_data);
short mainloop_pollmask_get(SOCKET fd);
void mainloop_pollmask_add(SOCKET fd, short events);
void mainloop_pollmask_del(SOCKET fd, short events);
void mainloop_pollmask_set(SOCKET fd, short events);