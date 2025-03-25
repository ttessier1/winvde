#pragma once

#include <WinSock2.h>
#include <stdint.h>

struct mod_support {
	char* modname;
	int (*sender)(SOCKET fd_ctl, SOCKET fd_data, void* packet, int len, uint16_t port);
	void (*delep)(SOCKET fd_ctl, SOCKET fd_data, void* descr);
};