// standard includes
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>
#include <winmeta.h>
#include <afunix.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <process.h>
#include <direct.h>

// custom includes
#include "winvde_mgmt.h"
#include "winvde_module.h"
#include "winvde_comlist.h"
#include "winvde_switch.h"
#include "winvde_output.h"
#include "winvde_printfunc.h"
#include "winvde_qtimer.h"
#include "winvde_loglevel.h"
#include "winvde_getopt.h"
#include "winvde_sockutils.h"
#include "version.h"
#include "winvde_user.h"
#include "winvde_descriptor.h"
#include "winvde_type.h"
#include "winvde_debugcl.h"
#include "winvde_memorystream.h"

//#ifdef DEBUGOPT
#define DBGCLSTEP 8

#define MGMTPORTNEW (dl) 
#define MGMTPORTDEL (dl+1) 
#define MGMTSIGHUP (dl+2) 
static struct dbgcl dl[] = {
	{"mgmt/+",NULL,D_MGMT | D_PLUS},
	{"mgmt/-",NULL,D_MGMT | D_MINUS},
	{"sig/hup",NULL,D_SIG | D_HUP}
};
//#endif

// defines
#define Nlong_options (sizeof(long_options)/sizeof(struct option));

#define MGMTMODEARG 0x100
#define MGMTGROUPARG 0x101
#define NOSTDINARG 0x102

int daemonize = 0;
int nostdin = 0;
unsigned int console_type = -1;
unsigned int mgmt_ctl = -1;
unsigned int mgmt_data = -1;
unsigned int mgmt_group = -1;

char pidfile_path[MAX_PATH];

// forward declarations
int vde_logout();
int vde_shutdown();
int mgmt_showinfo(struct comparameter* parameter);
void save_pidfile();
int runscript(SOCKET fd, char* path);
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

BOOL DoDaemonize = FALSE;
BOOL DoNoStdIn = FALSE;
BOOL DoPidFile = FALSE;
BOOL DoRcFile = FALSE;
BOOL DoMgmtSocket = FALSE;

char* pidfile = NULL;
char* rcfile = NULL;
char* mgmt_socket = NULL;

// function declarations
void mgmt_usage();
int mgmt_parse_options(const int c, const char* optarg);
void mgmt_init(void);
void mgmt_handle_io(unsigned char type, SOCKET fd, int revents, void* private_data);
void mgmt_cleanup(unsigned char type, SOCKET fd, void* private_data);
int handle_cmd(int type, SOCKET fd, char* inbuf);

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

int mgmt_parse_options(const int c, const char* optarg)
{
	int outc = 0;
	struct group* grp;
	switch (c)
	{
		case 'd':
			DoDaemonize = TRUE;
			break;
		case 'p':
			pidfile = _strdup(optarg);
			break;
		case 'f':
			rcfile = _strdup(optarg);
			break;
		case 'M':
			mgmt_socket = _strdup(optarg);
			break;
		case MGMTMODEARG:
			sscanf_s(optarg, "%o", &mgmt_mode);
			break;
		case MGMTGROUPARG:
			if (!(grp = GetGroupFunction(optarg)))
			{
				fprintf(stderr, "No such group '%s'\n", optarg);
				exit(1);
			}
			mgmt_group = grp->groupid;
			break;
		case NOSTDINARG:
			nostdin = 1;
			break;
		default:
			outc = c;
	}
	return outc;

}

void mgmt_init(void)
{
	if (DoDaemonize) {
		//openlog(basename(prog), LOG_PID, 0);
		//logok = 1;
		//syslog(LOG_INFO, "VDE_SWITCH started");
	}
	/* add stdin (if tty), connect and data fds to the set of fds we wait for
	 *    * input */

	if (!DoDaemonize && !DoNoStdIn)
	{
		console_type = add_type(&mgmgt_module, 0);
		add_fd(0, console_type, NULL);
	}

	/* saves current path in pidfile_path, because otherwise with daemonize() we
	 *    * forget it */
	if (_getcwd(pidfile_path, MAX_PATH - 2) == NULL) {
		strerror_s(errorbuff, sizeof(errorbuff), errno);
		printlog(LOG_ERR, "getcwd: %s", errorbuff);
		exit(1);
	}
	strcat_s(pidfile_path,MAX_PATH, "\\");
	if (DoDaemonize ) {

		RunDaemon();
		strerror_s(errorbuff, sizeof(errorbuff), errno);
		printlog(LOG_ERR, "daemon: %s", errorbuff);
		exit(1);
	}

	/* once here, we're sure we're the true process which will continue as a
	 *    * server: save PID file if needed */
	if (pidfile)
	{
		save_pidfile();
	}

	if (mgmt_socket != NULL) {
		SOCKET mgmtconnfd;
		struct sockaddr_un sun;
		int one = 1;

		if ((mgmtconnfd = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
			strerror_s(errorbuff, sizeof(errorbuff), errno);
			printlog(LOG_ERR, "mgmt socket: %s", errorbuff);
			return;
		}
		if (setsockopt(mgmtconnfd, SOL_SOCKET, SO_REUSEADDR, (char*)&one,sizeof(one)) < 0) {
			strerror_s(errorbuff, sizeof(errorbuff), errno);
			printlog(LOG_ERR, "mgmt setsockopt: %s", errorbuff);
			return;
		}
		if(ioctlsocket(mgmtconnfd, FIONBIO,&one)==SOCKET_ERROR)
		{
			strerror_s(errorbuff, sizeof(errorbuff), errno);
			printlog(LOG_ERR, "Setting O_NONBLOCK on mgmt fd: %s", errorbuff);
			return;
		}
		sun.sun_family = PF_UNIX;
		snprintf(sun.sun_path, sizeof(sun.sun_path), "%s", mgmt_socket);
		if (bind(mgmtconnfd, (struct sockaddr*)&sun, sizeof(sun)) < 0) {
			if ((errno == EADDRINUSE) && still_used(&sun)) return;
			else if (bind(mgmtconnfd, (struct sockaddr*)&sun, sizeof(sun)) < 0) {
				strerror_s(errorbuff, sizeof(errorbuff), errno);
				printlog(LOG_ERR, "mgmt bind %s", errorbuff);
				return;
			}
		}
		// Do not set permissions
		//setmgmtperm(sun.sun_path);
		if (listen(mgmtconnfd, 15) < 0) {
			strerror_s(errorbuff, sizeof(errorbuff), errno);
			printlog(LOG_ERR, "mgmt listen: %s", errorbuff);
			return;
		}
		mgmt_ctl = add_type(&mgmgt_module, 0);
		mgmt_data = add_type(&mgmgt_module, 0);
		add_fd(mgmtconnfd, mgmt_ctl, NULL);
	}

}

void mgmt_handle_io(unsigned char type, SOCKET fd, int revents, void* private_data)
{
	char buf[MAXCMD];
	int n = 0;
	int cmdout = 0;
	struct sockaddr addr;
	int one = 1;
	SOCKET new = INVALID_SOCKET;
	int len;
	if (type != mgmt_ctl)
	{
		if (revents & POLLIN)
		{
			n = recv(fd, buf, sizeof(buf),0);
			if (n < 0)
			{
				strerror_s(errorbuff, sizeof(errorbuff), errno);
				printlog(LOG_WARNING, "Reading from mgmt %s",errorbuff);
				return;
			}
		}
		if (n == 0)
		{ /*EOF || POLLHUP*/
			if (type == console_type)
			{
				printlog(LOG_WARNING, "EOF on stdin, cleaning up and exiting");
				exit(0);
			}
			else
			{
#ifdef DEBUGOPT
				debugdel(fd, "");
#endif
				remove_fd(fd);
			}
		}
		else
		{
			buf[n] = 0;
			if (n > 0 && buf[n - 1] == '\n')
			{
				buf[n - 1] = 0;
			}
			cmdout = handle_cmd(type, (type == console_type) ? 1 : fd, buf);
			if (cmdout >= 0)
			{
				send(fd, prompt, (int)strlen(prompt), 0);
			}
			else
			{
				if (type == mgmt_data)
				{
					send(fd, EOS, (int)strlen(EOS),0);
#ifdef DEBUGOPT
					EVENTOUT(MGMTPORTDEL, fd);
					debugdel(fd, "");
#endif
					remove_fd(fd);
				}
				if (cmdout == -2)
				{
					exit(0);
				}
			}
		}
	}
	else
	{/* mgmt ctl */
		len = sizeof(addr);
		new = accept(fd, &addr,  & len);
		if (new < 0)
		{
			strerror_s(errorbuff,sizeof(errorbuff),errno);
			printlog(LOG_WARNING, "mgmt accept %s", errorbuff);
			return;
		}
		if (ioctlsocket(new, FIONBIO, &one) == SOCKET_ERROR)
		{
			strerror_s(errorbuff, sizeof(errorbuff), errno);
			printlog(LOG_WARNING, "mgmt fcntl - setting O_NONBLOCK %s", errorbuff);
			closesocket(new);
			return;
		}

		add_fd(new, mgmt_data, NULL);
		EVENTOUT(MGMTPORTNEW, new);
		snprintf(buf, MAXCMD, header, PACKAGE_VERSION);
		send(new, buf, (int)strlen(buf),0);
		send(new, prompt, (int)strlen(prompt),0);
	}

}

void mgmt_cleanup(unsigned char type, SOCKET fd, void* private_data)
{
	if (fd < 0)
	{
		if ((pidfile != NULL) && _unlink(pidfile_path) < 0)
		{
			strerror_s(errorbuff, sizeof(errorbuff), errno);
			printlog(LOG_WARNING, "Couldn't remove pidfile '%s': %s", pidfile, errorbuff);
		}
	}
	else
	{
		closesocket(fd);
		if (type == mgmt_ctl && mgmt_socket != NULL) {
			_unlink(mgmt_socket);
		}
	}


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

int mgmt_showinfo(struct comparameter* parameter)
{
	if (parameter == NULL)
	{
		errno = EINVAL;
		return -1;
	}
	if (parameter->type1 == com_type_file )
	{
		printoutc(parameter->data1.file_descriptor, header, PACKAGE_VERSION);
		printoutc(parameter->data1.file_descriptor, "pid %d MAC %02x:%02x:%02x:%02x:%02x:%02x uptime %d", _getpid(),
			switchmac[0], switchmac[1], switchmac[2], switchmac[3], switchmac[4], switchmac[5],
			qtime());
		if (mgmt_socket)
		{
			printoutc(parameter->data1.file_descriptor, "mgmt %s perm 0%03o", mgmt_socket, mgmt_mode);
		}
	}
	return 0;
}

int runscript(SOCKET fd, char* path)
{
	size_t len = 0;
	FILE* f = fopen(path, "r");
	char buf[MAXCMD];
	if (f == NULL)
	{
		return errno;
	}
	else
	{
		while (fgets(buf, MAXCMD, f) != NULL)
		{
			if (strlen(buf) > 1 && buf[strlen(buf) - 1] == '\n')
			{
				buf[strlen(buf) - 1] = '\0';
			}
			if (fd >= 0)
			{
				char* scriptprompt = NULL;
				asprintf(&scriptprompt, "vde[%s]: %s\n", path, buf);
				len = strlen(scriptprompt);
				if (len > INT_MAX)
				{
					fprintf(stderr, "Failed to write more than INT_MAX bytes: %lld\n",len);
					return -1;
				}
				send(fd, scriptprompt, (int)len,0);
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

int handle_cmd(int type, SOCKET socketDescriptor, char* input_buffer)
{
	struct comlist* p;
	int rv = ENOSYS;
	char* outbuf=NULL;
	size_t outbufsize=0;
	struct _memory_stream * memStream = NULL;
	struct comparameter parameter;
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

		memStream = open_memorystream(&outbuf, &outbufsize);
		if (memStream)
		{

			
			for (p = clh; p != NULL && (p->doit == NULL || strncmp(p->path, input_buffer, strlen(p->path)) != 0); p = p->next)
			{
				;
			}
			if (p != NULL)
			{
				input_buffer += strlen(p->path);
				while (*input_buffer == ' ' || *input_buffer == '\t')
				{
					input_buffer++;
				}
				if (p->type & WITHFD)
				{
					if (socketDescriptor >= 0)
					{
						if (p->type & WITHFILE)
						{
							write_memorystream(memStream, "0000 DATA END WITH '.'", strlen("0000 DATA END WITH '.'"));
							parameter.type1 = com_type_memstream;
							parameter.data1.mem_stream = memStream;
							parameter.type2 = com_type_socket;
							parameter.data2.socket = socketDescriptor;
							switch (p->type & ~(WITHFILE | WITHFD))
							{
								case NOARG: 
									parameter.paramType = com_param_type_null;
									rv = p->doit(&parameter); 
								break;
								case INTARG: 
									parameter.paramType = com_param_type_int;
									parameter.paramValue.intValue = atoi(input_buffer);
									rv = p->doit(&parameter); 
								break;
								case STRARG: 
									parameter.paramType = com_param_type_string;
									parameter.paramValue.stringValue = input_buffer;
									rv = p->doit(parameter); 
								break;
							}
							write_memorystream(memStream, ".", strlen("."));
						}
						else
						{
							parameter.type1 = com_type_socket;
							parameter.data1.socket = socketDescriptor;
							parameter.type2 = com_type_null;
							switch (p->type & ~WITHFD)
							{
							case NOARG:
								parameter.paramType = com_param_type_null;
								rv = p->doit(&parameter); 
							break;
							case INTARG:
								parameter.paramType = com_param_type_int;
								parameter.paramValue.intValue = atoi(input_buffer);
								rv = p->doit(&parameter); 
							break;
							case STRARG:
								parameter.paramType = com_param_type_string;
								parameter.paramValue.stringValue = input_buffer;
								rv = p->doit(&parameter); 
							break;
							}
						}
					}
					else
					{
						rv = EBADF;
					}
				}
				else if (p->type & WITHFILE)
				{
					write_memorystream(memStream, "0000 DATA END WITH '.'", strlen("0000 DATA END WITH '.'"));\
					parameter.type1 = com_type_memstream;
					parameter.data1.mem_stream = memStream;
					parameter.type2 = com_type_null;
					switch (p->type & ~WITHFILE)
					{
					case NOARG: 
						parameter.paramType = com_param_type_null;
						rv = p->doit(&parameter); 
					break;
					case INTARG: 
						parameter.paramType = com_param_type_int;
						parameter.paramValue.intValue= atoi(input_buffer);
						rv = p->doit(&parameter); 
					break;
					case STRARG:
						parameter.paramType = com_param_type_string;
						parameter.paramValue.stringValue = input_buffer;
						rv = p->doit(&parameter); 
					break;
					}
					write_memorystream(memStream, ".", strlen("."));
				}
				else
				{
					parameter.type1 = com_type_null;
					parameter.type2 = com_type_null;
					switch (p->type)
					{
					case NOARG: 
						parameter.paramType = com_param_type_null;
						rv = p->doit(&parameter);
					break;
					case INTARG: 
						parameter.paramType = com_param_type_int;
						parameter.paramValue.intValue = atoi(input_buffer);
						rv = p->doit(&parameter); 
					break;
					case STRARG: 
						parameter.paramType = com_param_type_string;
						parameter.paramValue.stringValue = input_buffer;
						rv = p->doit(&parameter); 
					break;
					}
				}
			}
			if (rv == 0)
			{
				write_memorystream(memStream, "1000 Success", strlen("1000 Success"));
			}
			else if (rv > 0)
			{
				strerror_s(errorbuff, sizeof(errorbuff), errno);
				printoutc(memStream, "1%03d %s", rv, errorbuff);

				//write_memorystream(f, "1000 Success", strlen("1000 Success"));
			}

			if (socketDescriptor >= 0)
			{
				get_buffer(memStream);
				send(socketDescriptor, outbuf, outbufsize, 0);
			}
			close_memorystream(memStream);
			free(outbuf);
		}
		else
		{
			fprintf(stderr,"Failed to open the memory stream\n");
			errno = ENOMEM;
			return -1;
		}
	}
	return rv;
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


void RunDaemon()
{

}

static void save_pidfile()
{
	if (pidfile[0] != '/')
	{
		strncat_s(pidfile_path, MAX_PATH,pidfile, MAX_PATH - strlen(pidfile_path) - 1);
	}
	else
		strncpy_s(pidfile_path, MAX_PATH,pidfile, MAX_PATH - 1);

	
	FILE* fileHandle = NULL;
	errno = fopen_s(&fileHandle, pidfile_path, "w");
	if (errno == -1 || fileHandle==NULL) {
		strerror_s(errorbuff,sizeof(errorbuff),errno);
		printlog(LOG_ERR, "Error in pidfile creation: %s",errorbuff);
		exit(1);
	}

	if (fprintf(fileHandle, "%ld\n", (long int)_getpid()) <= 0) {
		printlog(LOG_ERR, "Error in writing pidfile");
		exit(1);
	}
	fclose(fileHandle);
}

/* void eventout(struct dbgcl* cl, ...)
{
	uint32_t index = 0;
	va_list arg;
	for (index = 0; index < cl->nfun; index++) {
		va_start(arg, cl);
		(cl->fun[index])(cl, cl->funarg[index], arg);
		va_end(arg);
	}
}
*/