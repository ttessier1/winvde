#pragma once

#include <stdio.h>

void printlog(int priority, const char* format, ...);
void printoutc(FILE* f, const char* format, ...);

#define DBGOUT(CL, FORMAT, ...) \
	if (((CL)->nfds) > 0) debugout((CL), (FORMAT), __VA_ARGS__)
#define EVENTOUT(CL, ...) \
	if (((CL)->nfun) > 0) eventout((CL), __VA_ARGS__)
#define PACKETFILTER(CL, PORT, BUF, LEN) \
	(((CL)->nfun) == 0 || ((LEN)=packetfilter((CL), (PORT), (BUF), (LEN))))
