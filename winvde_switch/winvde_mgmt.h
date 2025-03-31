#pragma once

#include <signal.h>
#include <stdint.h>

#include "winvde_mgmt.h"

#define MAXCMD 128

#define ADDCL(CL) addcl(sizeof(CL)/sizeof(struct comlist),(CL))

#define SIGHUP SIGTERM

extern uint32_t switch_errno;

void StartConsMgmt(void);
void loadrcfile(void);
void RunDaemon();
void setmgmtperm(char* path);
