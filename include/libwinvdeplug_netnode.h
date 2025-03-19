#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <libwinvde.h>
#include <winvdeplugmodule.h>

#define HUBNODE 1
#define MULTINODE 2
#define SWITCHNODE 3
#define BONDINGNODE 4


WINVDECONN* winvde_netnode_open(const char* vde_url, const char* descr, int interface_version, struct winvde_open_args* open_args, enum netnode_type nettype);

WINVDECONN* winvde_hub_open(const char* vde_url,const char* descr, int interface_version, struct winvde_open_args* open_args);

WINVDECONN* winvde_multi_open(const char* vde_url, const char* descr, int interface_version, struct winvde_open_args* open_args);

WINVDECONN* winvde_switch_open(const char* vde_url, const char* descr, int interface_version, struct winvde_open_args* open_args);

WINVDECONN* winvde_bonding_open(const char* vde_url, const char* descr, int interface_version, struct winvde_open_args* open_args);

size_t winvde_netnode_recv(WINVDECONN* conn, void* buf, size_t len, int flags);

/* There is an avent coming from the host */

size_t winvde_netnode_send(WINVDECONN* conn, const void* buf, size_t len, int flags);

int winvde_netnode_datafd(WINVDECONN* conn);

int winvde_netnode_ctlfd(WINVDECONN* conn);

int winvde_netnode_close(WINVDECONN* conn);

extern struct winvdeplug_module winvdeplug_hub_ops ;

extern struct winvdeplug_module winvdeplug_multi_ops ;

extern struct winvdeplug_module winvdeplug_switch_ops ;

extern struct winvdeplug_module winvdeplug_bonding_ops ;

