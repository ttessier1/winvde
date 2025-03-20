#pragma once

#include <WinSock2.h>
#include <stdint.h>

struct winvde_module {
	char* module_name; /* module name */
	char module_tag;   /* module tag - computer by the load sequence */
	uint16_t options; /* number of options for getopt */
	struct option* module_options; /* options for getopt */
	void (*usage)(void); /* usage function: command line opts explanation */
	int (*parse_options)(const int parm, const char* optarg); /* parse getopt output */
	void (*init)(void); /* init */
	void (*handle_io)(unsigned char type, SOCKET fd, int revents, void* private_data); /* handle input */
	void (*cleanup)(unsigned char type, SOCKET fd, void* private_data); /*cleanup for files or final if fd == -1 */
	struct winvde_module* next;
};

extern struct winvde_module* winvde_modules;

void add_module(struct winvde_module* new);
void del_module(struct winvde_module* old);
void init_mods(void);