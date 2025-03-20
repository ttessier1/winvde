#pragma once

#define MAXCMD 128

#define ADDCL(CL) addcl(sizeof(CL)/sizeof(struct comlist),(CL))


void StartConsMgmt(void);
void loadrcfile(void);
void RunDaemon();
