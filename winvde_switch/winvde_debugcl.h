#pragma once

#include <winsock2.h>
#include <stdarg.h>

#define D_PACKET 01000
#define D_MGMT 02000
#define D_SIG 03000
#define D_IN 01
#define D_OUT 02
#define D_PLUS 01
#define D_MINUS 02
#define D_DESCR 03
#define D_STATUS 04
#define D_ROOT 05
#define D_HASH 010
#define D_PORT 020
#define D_EP 030
#define D_FSTP 040
#define D_HUP 01

#if defined(DEBUGOPT)

typedef int (*intfun)(struct dbgcl*);

struct dbgcl {
	char* path; /* debug path for add/del */
	char* help; /* help string. just event mgmt when NULL */
	int tag;    /* tag for event mgmt and simple parsing */
	SOCKET * fds;   /* file descriptors for debug */
	intfun(*fun); /* function call or plugin events */
	void** funarg; /* arg for function calls */
	unsigned short nfds; /* number of active fds */
	unsigned short nfun; /* number of active fun */
	unsigned short maxfds; /* current size of fds */
	unsigned short maxfun; /* current size of both fun and funarg */
	struct dbgcl* next;
};

extern struct dbgcl* dbgclh;

extern struct dbgcl** dbgclt;

void adddbgcl(size_t ncl, struct dbgcl* cl);
void deldbgcl(size_t ncl, struct dbgcl* cl);


#endif