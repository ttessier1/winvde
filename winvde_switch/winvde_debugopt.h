#pragma once

#include <stdio.h>

#include "winvde_comlist.h"
#include "winvde_debugcl.h"


#if defined(DEBUGOPT)

int debuglist(struct comparameter * parameter);
int debugadd(struct comparameter* parameter);
int debugdel(struct comparameter* parameter);
void debugout(struct dbgcl* cl, const char* format, ...);

int packet_filter(struct dbgcl* cl, unsigned short port, char* buff, int length);

#define DBGOUT(CL, FORMAT, ...) if (((CL)->nfds) > 0) debugout((CL), (FORMAT), __VA_ARGS__)

//#define PACKETFILTER(CL, PORT, BUF, LEN) ((CL)->nfun) == 0 || ((LEN)=packetfilter((CL), (PORT), (BUF), (LEN)))

#else

#define DBGOUT(CL, ...)

//#define PACKETFILTER(CL, PORT, BUF, LEN)  (LEN)

#endif