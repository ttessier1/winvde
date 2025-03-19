#pragma once

#define NOARG 0
#define INTARG 1
#define STRARG 2
#define WITHFILE 0x40
#define WITHFD 0x80

struct comlist {
	char* path;
	char* syntax;
	char* help;
	int (*doit)();
	unsigned char type;
	struct comlist* next;
};

void addcl(int ncl, struct comlist* cl);
#define ADDCL(CL) addcl(sizeof(CL)/sizeof(struct comlist),(CL))
void delcl(int ncl, struct comlist* cl);

extern struct comlist* clh ;
extern struct comlist** clt;