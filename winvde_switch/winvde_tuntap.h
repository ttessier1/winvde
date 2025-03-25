#pragma once
#include <winsock2.h>

int send_tap(SOCKET fd_ctl, SOCKET fd_data, void* packet, int len, int port);
void start_tuntap(void);
