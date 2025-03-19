#pragma once

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

extern unsigned char switchmac[];
extern char* prog;
extern char errorbuff[1024];