#pragma once

#include <WinSock2.h>

struct mod_support {
	char* modname;
	int (*sender)(SOCKET fd_ctl, SOCKET fd_data, void* packet, int len, int port);
	void (*delep)(SOCKET fd_ctl, SOCKET fd_data, void* descr);
};