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
int eventadd(int (*fun)(), char* path, void* arg);
int eventdel(int (*fun)(), char* path, void* arg);
void eventout(struct dbgcl* cl, ...);

#endif