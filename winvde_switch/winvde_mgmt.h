#pragma once

#include <signal.h>

#define MAXCMD 128

#define ADDCL(CL) addcl(sizeof(CL)/sizeof(struct comlist),(CL))

#define SIGHUP SIGTERM

void StartConsMgmt(void);
void loadrcfile(void);
void RunDaemon();
void setmgmtperm(char* path);
