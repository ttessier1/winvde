#pragma once

struct mod_support {
	char* modname;
	int (*sender)(int fd_ctl, int fd_data, void* packet, int len, int port);
	void (*delep)(int fd_ctl, int fd_data, void* descr);
};