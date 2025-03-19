// standard includes
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

// custom includes
#include "winvde_datasock.h"
#include "winvde_module.h"
#include "winvde_modsupport.h"
#include "winvde_comlist.h"
#include "options.h"

// defines

#define DATA_BUF_SIZE 131072
#define SWITCH_MAGIC 0xfeedface
#define REQBUFLEN 256

#define DIRMODEARG	0x100

// globals
static struct option long_options[] = {
	{"sock", 1, 0, 's'},
	{"vdesock", 1, 0, 's'},
	{"unix", 1, 0, 's'},
	{"mod", 1, 0, 'm'},
	{"mode", 1, 0, 'm'},
	{"dirmode", 1, 0, DIRMODEARG},
	{"group", 1, 0, 'g'},
};

static char ctl_socket[MAX_PATH];

static int mode = -1;

#define Nlong_options (sizeof(long_options)/sizeof(struct option));

int datasock_showinfo(FILE* fd);

static struct comlist cl[] = {
	{"ds","============","DATA SOCKET MENU",NULL,NOARG},
	{"ds/showinfo","","show ds info",datasock_showinfo,NOARG | WITHFILE},
};

struct winvde_module datasock_module;

static struct mod_support modfun;

// function declarations


void datasock_init(void);
void datasock_usage(void);
int datasock_parse_options(const int c, const char** optarg);
void datasock_cleanup(unsigned char type, int fd, void* arg);

// Entry Point
void StartDataSock(void)
{
	datasock_module.module_name = "datasock";
	datasock_module.module_options = Nlong_options;
	datasock_module.options = long_options;
	datasock_module.usage = datasock_usage;
	datasock_module.parse_options = datasock_parse_options;
	datasock_module.init = datasock_init;
	datasock_module.handle_io;
	datasock_module.cleanup = datasock_cleanup;
	ADDCL(cl);
	add_module(&datasock_module);
}

// function definitions

int datasock_showinfo(FILE* fd)
{
	printoutc(fd, "ctl dir %s", ctl_socket);
	printoutc(fd, "std mode 0%03o", mode);
	return 0;
}

void datasock_init(void)
{
	fprintf(stderr, "init:Not Implemented\n");
	return;
}

void datasock_usage(void)
{
	printf(
		"(opts from datasock module)\n"
		"  -s, --sock SOCK            control directory pathname\n"
		"  -s, --vdesock SOCK         Same as --sock SOCK\n"
		"  -s, --unix SOCK            Same as --sock SOCK\n"
		"  -m, --mode MODE            Permissions for the control socket (octal)\n"
		"      --dirmode MODE         Permissions for the sockets directory (octal)\n"
		"  -g, --group GROUP          Group owner for comm sockets\n"
	);
}

int datasock_parse_options(const int c, const char** optarg)
{
	fprintf(stderr, "init:Not Implemented\n");
	return -1;
}

void datasock_cleanup(unsigned char type, int fd, void* arg)
{
	fprintf(stderr, "cleanup:Not Implemented\n");
	return ;
}