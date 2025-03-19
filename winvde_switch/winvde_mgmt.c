// standard includes
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winmeta.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <process.h>

// custom includes
#include "winvde_mgmt.h"
#include "winvde_module.h"
#include "winvde_comlist.h"
#include "winvde_switch.h"
#include "winvde_output.h"
#include "winvde_printfunc.h"
#include "winvde_qtimer.h"
#include "winvde_loglevel.h"
#include "options.h"
#include "version.h"

// defines
#define Nlong_options (sizeof(long_options)/sizeof(struct option));

#define MGMTMODEARG 0x100
#define MGMTGROUPARG 0x101
#define NOSTDINARG 0x102

#define EXIST_OK 0
#define W_OK 02
#define R_OK 04
#define RW_OK 06

// forward declarations
int vde_logout();
int vde_shutdown();
int mgmt_showinfo(FILE* fd);
int runscript(int fd, char* path);
int help(FILE* fd, char* arg);

static struct comlist cl[] = {
	{"help","[arg]","Help (limited to arg when specified)",help,STRARG | WITHFILE},
	{"logout","","logout from this mgmt terminal",vde_logout,NOARG},
	{"shutdown","","shutdown of the switch",vde_shutdown,NOARG},
	{"showinfo","","show switch version and info",mgmt_showinfo,NOARG | WITHFILE},
	{"load","path","load a configuration script",runscript,STRARG | WITHFD},
#ifdef DEBUGOPT
	{"debug","============","DEBUG MENU",NULL,NOARG},
	{"debug/list","","list debug categories",debuglist,STRARG | WITHFILE | WITHFD},
	{"debug/add","dbgpath","enable debug info for a given category",debugadd,WITHFD | STRARG},
	{"debug/del","dbgpath","disable debug info for a given category",debugdel,WITHFD | STRARG},
#endif
#ifdef VDEPLUGIN
	{"plugin","============","PLUGINS MENU",NULL,NOARG},
	{"plugin/list","","list plugins",pluginlist,STRARG | WITHFILE},
	{"plugin/add","library","load a plugin",pluginadd,STRARG},
	{"plugin/del","name","unload a plugin",plugindel,STRARG},
#endif
};

// globals
static struct option long_options[] = {
	{"daemon", 0, 0, 'd'},
	{"pidfile", 1, 0, 'p'},
	{"rcfile", 1, 0, 'f'},
	{"mgmt", 1, 0, 'M'},
	{"mgmtmode", 1, 0, MGMTMODEARG},
	{"mgmtgroup", 1, 0, MGMTGROUPARG},
	{"nostdin", 0, 0, NOSTDINARG},
#ifdef DEBUGOPT
	{"debugclients",1,0,'D'},
#endif
};

struct winvde_module mgmgt_module;

int mgmt_mode = 0600;
unsigned int mgmt_data = -1;

BOOL DoDaemonize = FALSE;
BOOL DoPidFile = FALSE;
BOOL DoRcFile = FALSE;
BOOL DoMgmtSocket = FALSE;

char* pidfile = NULL;
char* rcfile = NULL;
char* mgmt_socket = NULL;

// function declarations
void mgmt_usage();
int mgmt_parse_options(const int c, const char** optarg);
void mgmt_init(void);
void mgmt_handle_io(unsigned char type, int fd, int revents, void* private_data);
void mgmt_cleanup(unsigned char type, int fd, void* private_data);
int handle_cmd(int type, int fd, char* inbuf);

// EntryPoint
void StartConsMgmt(void)
{
	mgmgt_module.module_name = "console-mgmt";
	mgmgt_module.module_options = long_options;
	mgmgt_module.options = Nlong_options;
	mgmgt_module.usage = mgmt_usage;
	mgmgt_module.parse_options = mgmt_parse_options;
	mgmgt_module.init = mgmt_init;
	mgmgt_module.handle_io = mgmt_handle_io;
	mgmgt_module.cleanup = mgmt_cleanup;
	ADDCL(cl);
#if defined(DEBUGOPT)
	ADDDBCLI(dl);
#endif
	add_module(&mgmgt_module);
#if defined(DEBUGOPT)
	signal(SIGHUP, sighuopmgmt());
#endif
}

// function definitions

void mgmt_usage()
{
	fprintf(stderr,
		"(opts from consmgmt module)\n"
		"  -d, --daemon               Daemonize vde_switch once run\n"
		"  -p, --pidfile PIDFILE      Write pid of daemon to PIDFILE\n"
		"  -f, --rcfile               Configuration file (overrides %s and ~/.vderc)\n"
		"  -M, --mgmt SOCK            path of the management UNIX socket\n"
		"      --mgmtmode MODE        management UNIX socket access mode (octal)\n"
		"      --mgmtgroup GROUP      management UNIX socket group name\n"
		"      --nostdin              Allow stdin to be closed\n"
#ifdef DEBUGOPT
		"  -D, --debugclients #       number of debug clients allowed\n"
#endif
		, STDRCFILE

		);
}

int mgmt_parse_options(const int c, const char** optarg)
{
	fprintf(stderr, "parseopt:Not Implemeted\n");
	return -1;
}

void mgmt_init(void)
{
	fprintf(stderr, "init:Not Implemeted\n");
}

void mgmt_handle_io(unsigned char type, int fd, int revents, void* private_data)
{
	fprintf(stderr, "handle_io:Not Implemeted\n");
}

void mgmt_cleanup(unsigned char type, int fd, void* private_data)
{
	fprintf(stderr, "cleanup:Not Implemeted\n");

}

static int vde_logout()
{
	return -1;
}

static int vde_shutdown()
{
	printlog(LOG_WARNING, "Shutdown from mgmt command");
	return -2;
}

int mgmt_showinfo(FILE* fd)
{
	printoutc(fd, header, PACKAGE_VERSION);
	printoutc(fd, "pid %d MAC %02x:%02x:%02x:%02x:%02x:%02x uptime %d", _getpid(),
		switchmac[0], switchmac[1], switchmac[2], switchmac[3], switchmac[4], switchmac[5],
		qtime());
	if (mgmt_socket)
		printoutc(fd, "mgmt %s perm 0%03o", mgmt_socket, mgmt_mode);
	return 0;
}

int runscript(int fd, char* path)
{
	size_t len = 0;
	FILE* f = fopen(path, "r");
	char buf[MAXCMD];
	if (f == NULL)
		return errno;
	else {
		while (fgets(buf, MAXCMD, f) != NULL) {
			if (strlen(buf) > 1 && buf[strlen(buf) - 1] == '\n') buf[strlen(buf) - 1] = '\0';
			if (fd >= 0) {
				char* scriptprompt = NULL;
				asprintf(&scriptprompt, "vde[%s]: %s\n", path, buf);
				len = strlen(scriptprompt);
				if (len > INT_MAX)
				{
					fprintf(stderr, "Failed to write more than INT_MAX bytes: %lld\n",len);
					return -1;
				}
				_write(fd, scriptprompt, (int)len);
				free(scriptprompt);
			}
			handle_cmd(mgmt_data, fd, buf);
		}
		fclose(f);
		return 0;
	}
}

int help(FILE* fd, char* arg)
{
	struct comlist* p;
	size_t n = strlen(arg);
	printoutc(fd, "%-18s %-15s %s", "COMMAND PATH", "SYNTAX", "HELP");
	printoutc(fd, "%-18s %-15s %s", "------------", "--------------", "------------");
	for (p = clh; p != NULL; p = p->next)
		if (strncmp(p->path, arg, n) == 0)
			printoutc(fd, "%-18s %-15s %s", p->path, p->syntax, p->help);
	return 0;
}

int handle_cmd(int type, int fd, char* input_buffer)
{
	/*struct comlist* p;
	int rv = ENOSYS;
	if (!input_buffer)
	{
		errno = EINVAL;
		return -1;
	}
	while (*input_buffer == ' ' || *input_buffer == '\t')
	{
		input_buffer++;
	}
	if (*input_buffer != '\0' && *input_buffer != '#')
	{
		char* outbuf;
		size_t outbufsize;
		FILE* f = open_memstream(&outbuf, &outbufsize);
		for (p = clh; p != NULL && (p->doit == NULL || strncmp(p->path, input_buffer, strlen(p->path)) != 0); p = p->next)
			;
		if (p != NULL)
		{
			input_buffer += strlen(p->path);
			while (*input_buffer == ' ' || *input_buffer == '\t') input_buffer++;
			if (p->type & WITHFD) {
				if (fd >= 0) {
					if (p->type & WITHFILE) {
						printoutc(f, "0000 DATA END WITH '.'");
						switch (p->type & ~(WITHFILE | WITHFD)) {
						case NOARG: rv = p->doit(f, fd); break;
						case INTARG: rv = p->doit(f, fd, atoi(input_buffer)); break;
						case STRARG: rv = p->doit(f, fd, input_buffer); break;
						}
						printoutc(f, ".");
					}
					else {
						switch (p->type & ~WITHFD) {
						case NOARG: rv = p->doit(fd); break;
						case INTARG: rv = p->doit(fd, atoi(input_buffer)); break;
						case STRARG: rv = p->doit(fd, input_buffer); break;
						}
					}
				}
				else
					rv = EBADF;
			}
			else if (p->type & WITHFILE) {
				printoutc(f, "0000 DATA END WITH '.'");
				switch (p->type & ~WITHFILE) {
				case NOARG: rv = p->doit(f); break;
				case INTARG: rv = p->doit(f, atoi(input_buffer)); break;
				case STRARG: rv = p->doit(f, input_buffer); break;
				}
				printoutc(f, ".");
			}
			else {
				switch (p->type) {
				case NOARG: rv = p->doit(); break;
				case INTARG: rv = p->doit(atoi(input_buffer)); break;
				case STRARG: rv = p->doit(input_buffer); break;
				}
			}
		}
		if (rv == 0) {
			printoutc(f, "1000 Success");
		}
		else if (rv > 0)
		{
			strerror_s(errorbuff, sizeof(errorbuff), errno);
			printoutc(f, "1%03d %s", rv, errorbuff);
		}
		fclose(f);
		if (fd >= 0)
			write(fd, outbuf, outbufsize);
		free(outbuf);
	}
	return rv;
	*/
	return -1;

}
void loadrcfile(void)
{
	size_t size;
	if (rcfile != NULL)
		runscript(-1, rcfile);
	else {
		char path[MAX_PATH];
		char home[MAX_PATH];
		getenv_s(&size, home, sizeof(home), "USERPROFILE");
		snprintf(path, MAX_PATH, "%s\\.vde2\\vde_switch.rc", home);
		if (_access(path, R_OK) == 0)
			runscript(-1, path);
		else {
			if (_access(STDRCFILE, R_OK) == 0)
				runscript(-1, STDRCFILE);
		}
	}
}
