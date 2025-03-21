#pragma once

#include "winvde_module.h"

#define MAXFDS_INITIAL 8
#define MAXFDS_STEP 16

#define FDPERMSIZE_LOGSTEP 4

extern int maxfds;
extern int nprio;

struct pollplus {
	unsigned char type;
	void* private_data;
	time_t timestamp;
};

extern struct pollfd* fds;

extern struct pollplus** fdpp ;

extern short* fdperm;

extern uint64_t fdpermsize;

#define TYPEMASK 0x7f

#define TYPE2MGR(X) (winvde_modules[(X) & TYPEMASK])

extern uint32_t number_of_filedescriptors;

void add_fd(SOCKET fd, unsigned char type, void* private_data);
void remove_fd(SOCKET fd);
void FileCleanUp();