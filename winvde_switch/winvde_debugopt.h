#pragma once

#include <stdio.h>

#define DBGCLSTEP 8

#if defined(DEBUGOPT)

//int debuglist(FILE* f, int fd, char* path);
int debuglist(struct comparameter * parameter);
//int debugadd(int fd, char* path);
int debugadd(struct comparameter* parameter);
//
int debugdel(struct comparameter* parameter);
void debugout(struct dbgcl* cl, const char* format, ...);

int packetfilter(struct dbgcl* cl, ...);

#define DBGOUT(CL, FORMAT, ...) \
	if (((CL)->nfds) > 0) debugout((CL), (FORMAT), __VA_ARGS__)

#define PACKETFILTER(CL, PORT, BUF, LEN) \
	(((CL)->nfun) == 0 || ((LEN)=packetfilter((CL), (PORT), (BUF), (LEN))))

#else

#define DBGOUT(CL, ...)

#define PACKETFILTER(CL, PORT, BUF, LEN)  (LEN)

#endif