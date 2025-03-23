#pragma once

#if defined(DEBUGOPT)

#define DBGCLSTEP 8

int eventadd(int (*fun)(struct dbgcl*), char* path, void* arg);
int eventdel(int (*fun)(struct dbgcl*), char* path, void* arg);
void eventout(struct dbgcl* cl, ...);

#define EVENTOUT(CL, ...) \
	if (((CL)->nfun) > 0) \
		eventout((CL), __VA_ARGS__)

#else

#define EVENTOUT(CL, ...)

#endif
