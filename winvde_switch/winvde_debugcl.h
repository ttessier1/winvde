#pragma once

//#ifdef DEBUGOPT
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

typedef int (*intfun)();



struct dbgcl {
	char* path; /* debug path for add/del */
	char* help; /* help string. just event mgmt when NULL */
	int tag;    /* tag for event mgmt and simple parsing */
	int* fds;   /* file descriptors for debug */
	intfun(*fun); /* function call or plugin events */
	void** funarg; /* arg for function calls */
	unsigned short nfds; /* number of active fds */
	unsigned short nfun; /* number of active fun */
	unsigned short maxfds; /* current size of fds */
	unsigned short maxfun; /* current size of both fun and funarg */
	struct dbgcl* next;
};

struct dbgcl* dbgclh;

struct dbgcl** dbgclt;

void adddbgcl(int ncl, struct dbgcl* cl);
#define ADDDBGCL(CL) adddbgcl(sizeof(CL)/sizeof(struct dbgcl),(CL))
void debugout(struct dbgcl* cl, const char* format, ...);
void eventout(struct dbgcl* cl, ...);
int packetfilter(struct dbgcl* cl, ...);
#define DBGOUT(CL, FORMAT, ...) \
	if (((CL)->nfds) > 0) debugout((CL), (FORMAT), __VA_ARGS__)
#define EVENTOUT(CL, ...) \
	if (((CL)->nfun) > 0) eventout((CL), __VA_ARGS__)
#define PACKETFILTER(CL, PORT, BUF, LEN) \
	(((CL)->nfun) == 0 || ((LEN)=packetfilter((CL), (PORT), (BUF), (LEN))))
/*
#define PACKETFILTER(CL, PORT, BUF, LEN)  (LEN)
	*/
/* #else
#define DBGOUT(CL, ...) 
#define EVENTOUT(CL, ...) 
#define PACKETFILTER(CL, PORT, BUF, LEN)  (LEN)  
#endif */

void adddbgcl(int ncl, struct dbgcl* cl);
void deldbgcl(int ncl, struct dbgcl* cl);
void debugout(struct dbgcl* cl, const char* format, ...);
void eventout(struct dbgcl* cl, ...);
int packetfilter(struct dbgcl* cl, ...);