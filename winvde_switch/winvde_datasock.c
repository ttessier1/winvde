// standard includes
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winsock2.h>
#include <winmeta.h>
#include <afunix.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <direct.h>

// custom includes
#include "winvde_datasock.h"
#include "winvde_module.h"
#include "winvde_modsupport.h"
#include "winvde_comlist.h"
#include "winvde_getopt.h"
#include "winvde_loglevel.h"
#include "winvde_switch.h"
#include "winvde_common.h"
#include "winvde.h"
#include "winvde_sockutils.h"
#include "winvde_type.h"
#include "winvde_output.h"
#include "winvde_user.h"
#include "winvde_descriptor.h"
#include "winvde_sockutils.h"

// defines

#define DATA_BUF_SIZE 131072
#define SWITCH_MAGIC 0xbeefface
#define REQBUFLEN 256

#define DIRMODEARG	0x100

#define ADDBIT(mode, conditionmask, add) ((mode & conditionmask) ? ((mode & conditionmask) | add) : (mode & conditionmask))

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

char* rel_ctl_socket = NULL;
char ctl_socket[MAX_PATH];

uint32_t grp_owner = -1;

unsigned int ctl_type;
unsigned int wd_type;
unsigned int data_type;

#define MODULENAME "win prog"

int mode = -1;
int dirmode = -1;

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
int datasock_parse_options(const int c, const char* optarg);
void datasock_cleanup(unsigned char type, int fd, void* arg);

// Entry Point
void StartDataSock(void)
{
	datasock_module.module_name = "datasock";
	datasock_module.module_options = long_options;
	datasock_module.options = Nlong_options;
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
	SOCKET connect_fd;
	struct sockaddr_un sun;
	int one = 1;
	const size_t max_ctl_sock_len = sizeof(sun.sun_path) - 5;

	if (mode < 0 && dirmode < 0)
	{
		// default values
		mode = 00600;
		dirmode = 02700;
	}
	else if (mode >= 0 && dirmode < 0)
	{
		dirmode = 02000 |
			ADDBIT(mode, 0600, 0100) |
			ADDBIT(mode, 0060, 0010) |
			ADDBIT(mode, 0006, 0001);
	}
	else if (mode < 0 && dirmode >= 0)
	{
		mode = dirmode & 0666;
	}
	if (connect_fd = socket(AF_UNIX, SOCK_STREAM, 0) == INVALID_SOCKET)
	{
		strerror_s(errorbuff, sizeof(errorbuff), errno);
		printlog(LOG_ERR, "Could not obtain a socket %s\n",errorbuff);
		return;
	}
	/*if (setsockopt(connect_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&one, sizeof(one)) < 0)
	{
		strerror_s(errorbuff, sizeof(errorbuff), errno);
		printlog(LOG_ERR, "Could not setsocket SO_REUSEADDR option %s\n", errorbuff);
		return;
	}*/
	if (ioctlsocket(connect_fd, FIONBIO, &one) < 0)
	{
		strerror_s(errorbuff, sizeof(errorbuff), errno);
		printlog(LOG_ERR, "Could not set non blocking mode on socket %s\n", errorbuff);
		return;
	}
	if (rel_ctl_socket == NULL)
	{
		rel_ctl_socket = GetUserIdFunction()==0 ? VDESTDSOCK : VDETMPSOCK;
	}
	if (_mkdir(rel_ctl_socket)<0 && errno != EEXIST)
	{
		strerror_s(errorbuff, sizeof(errorbuff), errno);
		fprintf(stderr, "Failed to make the directory: %s\n", errorbuff);
		return;
	}
	if (!winvde_realpath(rel_ctl_socket,ctl_socket))
	{
		strerror_s(errorbuff, sizeof(errorbuff), errno);
		fprintf(stderr, "Can not resolve ctl dir path: '%s' %s\n", rel_ctl_socket, errorbuff);
		return;
	}
	fprintf(stdout, "Real Path of Socket: %s\n", ctl_socket);

	/*if (chown(ctl_socket, -1, grp_owner) < 0) {
		rmdir(ctl_socket);
		printlog(LOG_ERR, "Could not chown socket '%s': %s", sun.sun_path, strerror(errno));
		exit(-1);
	}
	if (chmod(ctl_socket, dirmode) < 0) {
		printlog(LOG_ERR, "Could not set the VDE ctl directory '%s' permissions: %s", ctl_socket, strerror(errno));
		exit(-1);
	}*/
	sun.sun_family = AF_UNIX;
	if (strlen(ctl_socket) > max_ctl_sock_len)
	{
		ctl_socket[max_ctl_sock_len] = 0;
	}
	snprintf(sun.sun_path, sizeof(sun.sun_path), "%s\\ctl.sock", ctl_socket);
	if (bind(connect_fd, (struct sockaddr*)&sun, sizeof(sun)) < 0) {
		if ((errno == EADDRINUSE) && still_used(&sun)) {

			strerror_s(errorbuff,sizeof(errorbuff),errno);
			printlog(LOG_ERR, "Could not bind to socket '%s\\ctl.sock': %s", ctl_socket, errorbuff);
			exit(-1);
		}
		else if (bind(connect_fd, (struct sockaddr*)&sun, sizeof(sun)) < 0) {
			strerror_s(errorbuff, sizeof(errorbuff), errno);
			printlog(LOG_ERR, "Could not bind to socket '%s\\ctl.sock' (second attempt): %s", ctl_socket, errorbuff);
			exit(-1);
		}
	}
	/*chmod(sun.sun_path, mode);
	if (chown(sun.sun_path, -1, grp_owner) < 0) {
		printlog(LOG_ERR, "Could not chown socket '%s': %s", sun.sun_path, strerror(errno));
		exit(-1);
	}*/
	if (listen(connect_fd, 15) < 0) {
		strerror_s(errorbuff, sizeof(errorbuff), errno);
		printlog(LOG_ERR, "Could not listen on fd %d: %s", connect_fd, errorbuff);
		exit(-1);
	}
	ctl_type = add_type(&datasock_module, 0);
	wd_type = add_type(&datasock_module, 0);
	data_type = add_type(&datasock_module, 1);
	add_fd(connect_fd, ctl_type, NULL);
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

int datasock_parse_options(const int c, const char* optarg)
{
	int outc = 0;
	struct group* grp = NULL;
	switch (c) {
		case 's':
			if (!(rel_ctl_socket = _strdup(optarg))) {
				fprintf(stderr, "Memory error while parsing '%s'\n", optarg);
				exit(1);
			}
			break;
		case 'm':
			sscanf_s(optarg, "%o", &mode);
			break;
		case 'g':
			if (!(grp = GetGroupFunction(optarg))) {
				fprintf(stderr, "No such group '%s'\n", optarg);
				exit(1);
			}
			grp_owner = grp->groupid;
			break;
		case DIRMODEARG:
			sscanf_s(optarg, "%o", &dirmode);
			break;
		default:
			outc = c;
	}
	return outc;

}

void datasock_cleanup(unsigned char type, SOCKET fd, void* arg)
{
	struct sockaddr_un clun;
	SOCKET test_fd;

	if (fd != INVALID_SOCKET){
		const size_t max_ctl_sock_len = sizeof(clun.sun_path) - 5;
		if (!strlen(ctl_socket)) {
			return;
		}
		if ((test_fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
			strerror_s(errorbuff,sizeof(errorbuff),errno);
			printlog(LOG_ERR, "socket %s", errorbuff);
		}
		clun.sun_family = AF_UNIX;
		if (strlen(ctl_socket) > max_ctl_sock_len)
		{
			ctl_socket[max_ctl_sock_len] = 0;
		}
		snprintf(clun.sun_path, sizeof(clun.sun_path), "%s/ctl", ctl_socket);
		if (connect(test_fd, (struct sockaddr*)&clun, sizeof(clun)))
		{
			closesocket(test_fd);
			if (_unlink(clun.sun_path) < 0)
			{
				strerror_s(errorbuff, sizeof(errorbuff), errno);
				printlog(LOG_WARNING, "Could not remove ctl socket '%s': %s", ctl_socket, errorbuff);
			}
			else if (_rmdir(ctl_socket) < 0)
			{
				strerror_s(errorbuff, sizeof(errorbuff), errno);
				printlog(LOG_WARNING, "Could not remove ctl dir '%s': %s", ctl_socket, errorbuff);
			}
		}
		else
		{
			printlog(LOG_WARNING, "Cleanup not removing files");
		}
	}
	else
	{
		if (type == data_type && arg != NULL)
		{
			int portno = ep_get_port(arg);
			const size_t max_ctl_sock_len = sizeof(clun.sun_path) - 8;
			if (strlen(ctl_socket) > max_ctl_sock_len)
			{
				ctl_socket[max_ctl_sock_len] = 0;
			}
			snprintf(clun.sun_path, sizeof(clun.sun_path), "%s\\%03d.%lld", ctl_socket, portno, fd);
			_unlink(clun.sun_path);
		}
		closesocket(fd);
	}

}