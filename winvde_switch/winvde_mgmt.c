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
#include "winvde_debugopt.h"
#include "winvde_event.h"
#include "winvde_memorystream.h"
#include "winvde_plugin.h"

#if defined(DEBUGOPT)

#define MGMTPORTNEW (mgmt_dl) 
#define MGMTPORTDEL (mgmt_dl+1) 
#define MGMTSIGHUP (mgmt_dl+2) 
static struct dbgcl mgmt_dl[] = {
	{"mgmt/+",NULL,D_MGMT | D_PLUS},
	{"mgmt/-",NULL,D_MGMT | D_MINUS},
	{"sig/hup",NULL,D_SIG | D_HUP}
};
#endif

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
uint32_t switch_errno = 0;

char pidfile_path[MAX_PATH];

// forward declarations
int help(struct comparameter* parameter);
int vde_logout(struct comparameter* parameter);
int vde_shutdown(struct comparameter* parameter);
int mgmt_showinfo(struct comparameter* parameter);
int validatecommand(char* command);
int validatescript(struct comparameter* parameter);
int runscript(struct comparameter* parameter);
void save_pidfile();
void sighupmgmt(int signo);
void mgmtnewfd(SOCKET new);






static struct comlist mgmt_cl[] = {
	{"help","[arg]","Help (limited to arg when specified)",help,STRARG | WITHFILE},
	{"logout","","logout from this mgmt terminal",vde_logout,NOARG},
	{"shutdown","","shutdown of the switch",vde_shutdown,NOARG},
	{"showinfo","","show switch version and info",mgmt_showinfo,NOARG | WITHFILE},
	{"load","path","load a configuration script",runscript,STRARG | WITHFD},
#if defined(DEBUGOPT)
	{"debug","============","DEBUG MENU",NULL,NOARG},
	{"debug/list","","list debug categories",debuglist,STRARG | WITHFILE | WITHFD},
	{"debug/add","dbgpath","enable debug info for a given category",debugadd,WITHFD | STRARG},
	{"debug/del","dbgpath","disable debug info for a given category",debugdel,WITHFD | STRARG},
#endif
#if defined(VDEPLUGIN)
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

// private function declarations
void mgmt_usage();
int mgmt_parse_options(const int c, const char* optarg);
int mgmt_init();
int mgmt_handle_io(unsigned char type, SOCKET fd, int revents, void* private_data);
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
	ADDCL(mgmt_cl);
#if defined(DEBUGOPT)
	adddbgcl(sizeof(mgmt_dl) / sizeof(struct dbgcl), mgmt_dl);
#endif
	add_module(&mgmgt_module);
#if defined(DEBUGOPT)
	signal(SIGHUP, sighupmgmt);
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

int mgmt_init()
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
		add_fd(INVALID_SOCKET, console_type, mgmgt_module.module_tag, NULL);
		/* This file descriptor is a problem for WSAPoll*/
	}

	/* saves current path in pidfile_path, because otherwise with daemonize() we
	 *    * forget it */
	if (_getcwd(pidfile_path, MAX_PATH - 2) == NULL) {
		strerror_s(errorbuff, sizeof(errorbuff), switch_errno);
		printlog(LOG_ERR, "getcwd: %s", errorbuff);
		exit(1);
	}
	strcat_s(pidfile_path,MAX_PATH, "\\");
	if (DoDaemonize ) {

		RunDaemon();
		strerror_s(errorbuff, sizeof(errorbuff), switch_errno);
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
			strerror_s(errorbuff, sizeof(errorbuff), switch_errno);
			printlog(LOG_ERR, "mgmt socket: %s", errorbuff);
			return -1;
		}
		
		/*if (setsockopt(mgmtconnfd, SOL_SOCKET, SO_REUSEADDR, (char*)&one, sizeof(one)) < 0) {
			strerror_s(errorbuff, sizeof(errorbuff), switch_errno);
			printlog(LOG_ERR, "mgmt setsockopt: %s", errorbuff);
			return;
		}*/
		if(ioctlsocket(mgmtconnfd, FIONBIO,&one)==SOCKET_ERROR)
		{
			strerror_s(errorbuff, sizeof(errorbuff), switch_errno);
			printlog(LOG_ERR, "Setting O_NONBLOCK on mgmt fd: %s", errorbuff);
			return -1;
		}
		sun.sun_family = PF_UNIX;
		snprintf(sun.sun_path, sizeof(sun.sun_path), "%s", mgmt_socket);
		if (bind(mgmtconnfd, (struct sockaddr*)&sun, sizeof(sun)) < 0) {
			if ((switch_errno == EADDRINUSE) && still_used(&sun))
			{
				return -1;
			}
			else if (bind(mgmtconnfd, (struct sockaddr*)&sun, sizeof(sun)) < 0) {
				strerror_s(errorbuff, sizeof(errorbuff), switch_errno);
				printlog(LOG_ERR, "mgmt bind %s", errorbuff);
				return -1;
			}
		}
		// Do not set permissions
		//setmgmtperm(sun.sun_path);
		if (listen(mgmtconnfd, 15) < 0) {
			strerror_s(errorbuff, sizeof(errorbuff), switch_errno);
			printlog(LOG_ERR, "mgmt listen: %s", errorbuff);
			return -1;
		}
		mgmt_ctl = add_type(&mgmgt_module, 0);
		mgmt_data = add_type(&mgmgt_module, 0);
		add_fd(mgmtconnfd, mgmt_ctl, mgmgt_module.module_tag, NULL);
	}
	else
	{
		fprintf(stdout, "WARNING: Skipping management socket\n");
		return -1;
	}
	return 0;
}

int mgmt_handle_io(unsigned char type, SOCKET fd, int revents, void* private_data)
{
	char buf[MAXCMD];
	int n = 0;
	int cmdout = 0;
	struct sockaddr addr;
	int one = 1;
	SOCKET new = INVALID_SOCKET;
	struct comparameter parameter = { 0 };
	int len;
	if (type != mgmt_ctl)
	{
		if (revents & POLLIN)
		{
			n = recv(fd, buf, sizeof(buf),0);
			if (n < 0)
			{
				strerror_s(errorbuff, sizeof(errorbuff), switch_errno);
				printlog(LOG_WARNING, "Reading from mgmt %s",errorbuff);
				return -1;
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
				fprintf(stderr,"File descriptor closed\n");
#ifdef DEBUGOPT
				parameter.type1 = com_type_socket;
				parameter.data1.socket = fd;
				parameter.type2 = com_type_null;
				parameter.paramType = com_param_type_string;
				parameter.paramValue.stringValue = "";
				debugdel(&parameter);
#endif
				remove_fd(fd);
			}
		}
		else
		{
			if (n < sizeof(buf))
			{
				buf[n] = 0;
			}
			if (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r'))
			{
				buf[n - 1] = 0;
			}
			cmdout = handle_cmd(type, (type == console_type) ? 1 : fd, buf);
			if (cmdout >= 0)
			{
				send(fd, prompt, strlength(prompt), 0);
			}
			else
			{
				if (type == mgmt_data)
				{
					send(fd, EOS, strlength(EOS),0);
#ifdef DEBUGOPT
					EVENTOUT(MGMTPORTDEL, fd);
					parameter.type1 = com_type_socket;
					parameter.data1.socket = fd;
					parameter.type2 = com_type_null;
					parameter.paramType = com_param_type_string;
					parameter.paramValue.stringValue = "";
					debugdel(&parameter);
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
		// This is the first management socket response
		len = sizeof(addr);
		new = accept(fd, NULL, NULL);
		if (new == INVALID_SOCKET)
		{
			strerror_s(errorbuff,sizeof(errorbuff),switch_errno);
			printlog(LOG_WARNING, "mgmt accept %s", errorbuff);
			return -1;
		}
		if (ioctlsocket(new, FIONBIO, &one) == SOCKET_ERROR)
		{
			strerror_s(errorbuff, sizeof(errorbuff), switch_errno);
			printlog(LOG_WARNING, "mgmt fcntl - setting O_NONBLOCK %s", errorbuff);
			closesocket(new);
			return -1;
		}

		add_fd(new, mgmt_data, mgmgt_module.module_tag, NULL);
		eventout(MGMTPORTNEW, new);
		snprintf(buf, MAXCMD, header, PACKAGE_VERSION);
		send(new, buf, strlength(buf),0);
		send(new, prompt, strlength(prompt),0);
	}
	return 0;
}

void mgmt_cleanup(unsigned char type, SOCKET fd, void* private_data)
{
	if (fd == INVALID_SOCKET)
	{
		if ((pidfile != NULL) && _unlink(pidfile_path) < 0)
		{
			strerror_s(errorbuff, sizeof(errorbuff), switch_errno);
			printlog(LOG_WARNING, "Couldn't remove pidfile '%s': %s", pidfile, errorbuff);
		}
	}
	else
	{
		closesocket(fd);
		if (type == mgmt_ctl && mgmt_socket != NULL || mgmt_ctl==0xFFFFFFFF && mgmt_socket != NULL) {
			_unlink(mgmt_socket);
		}
	}


}

int vde_logout(struct comparameter * parameter)
{
	if (!parameter)
	{
		switch_errno = EINVAL;
		return - 1;
	}
	if (parameter->type1 == com_type_null && parameter->type2 == com_type_null && parameter->paramType == com_param_type_null)
	{
		printlog(LOG_WARNING, "Logout from mgmt command");
		return -1;
	}
	return 0;
}

int vde_shutdown(struct comparameter* parameter)
{
	if (!parameter)
	{
		switch_errno = EINVAL;
		return -1;
	}
	if (parameter->type1 == com_type_null && parameter->type2 == com_type_null && parameter->paramType== com_param_type_null)
	{
		printlog(LOG_WARNING, "Shutdown from mgmt command");
		return -2;
	}
	switch_errno = EINVAL;
	return -1;
}

int mgmt_showinfo(struct comparameter* parameter)
{
	char* tmpBuff = NULL;
	size_t length;
	if (parameter == NULL)
	{
		switch_errno = EINVAL;
		return -1;
	}
	if (parameter->type1 == com_type_file && parameter->data1.file_descriptor!=NULL)
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
	else if (parameter->type1 == com_type_socket && parameter->data1.socket != INVALID_SOCKET)
	{
		length = asprintf(&tmpBuff, header, PACKAGE_VERSION);
		if (length && tmpBuff)
		{
			send(parameter->data1.socket, tmpBuff, (int)length, 0);
			free(tmpBuff);
		}
		else
		{
			switch_errno = ENOMEM;
			return -1;
		}
		length = asprintf(&tmpBuff, "pid %d MAC %02x:%02x:%02x:%02x:%02x:%02x uptime %d\n", _getpid(),
			switchmac[0], switchmac[1], switchmac[2], switchmac[3], switchmac[4], switchmac[5],
			qtime());
		if (length && tmpBuff)
		{
			send(parameter->data1.socket, tmpBuff, (int)length, 0);
			free(tmpBuff);
		}
		else
		{
			switch_errno = ENOMEM;
			return -1;
		}
		length = asprintf(&tmpBuff, "mgmt %s perm 0%03o\n", mgmt_socket, mgmt_mode);
		if (mgmt_socket)
		{
			if (length && tmpBuff)
			{
				send(parameter->data1.socket, tmpBuff, (int)length, 0);
				free(tmpBuff);
			}
			else
			{
				switch_errno = ENOMEM;
				return -1;
			}
		}
	}
	else if (parameter->type1 == com_type_memstream && parameter->data1.mem_stream != NULL)
	{
		length = asprintf(&tmpBuff, header, PACKAGE_VERSION);
		if (length && tmpBuff)
		{
			write_memorystream(parameter->data1.mem_stream, tmpBuff, length);
			free(tmpBuff);
		}
		else
		{
			switch_errno = ENOMEM;
			return -1;
		}
		length = asprintf(&tmpBuff, "pid %d MAC %02x:%02x:%02x:%02x:%02x:%02x uptime %d\n", _getpid(),
			switchmac[0], switchmac[1], switchmac[2], switchmac[3], switchmac[4], switchmac[5],
			qtime());
		if (length && tmpBuff)
		{
			write_memorystream(parameter->data1.mem_stream, tmpBuff, length);
			free(tmpBuff);
		}
		else
		{
			switch_errno = ENOMEM;
			return -1;
		}
		length = asprintf(&tmpBuff, "mgmt %s perm 0%03o\n", mgmt_socket, mgmt_mode);
		if (mgmt_socket)
		{
			if (length && tmpBuff)
			{
				write_memorystream(parameter->data1.mem_stream, tmpBuff, length);
				free(tmpBuff);
			}
			else
			{
				switch_errno = ENOMEM;
				return -1;
			}
		}
	}
	return 0;
}

int validatecommand(char * command)
{
	struct comlist* p = NULL;
	if (!command)
	{
		switch_errno = EINVAL;
 		return -1;
	}

	for (p = clh; p != NULL && (p->doit == NULL || strncmp(p->path, command, strlen(p->path)) != 0); p = p->next)
	{
		;
	}
	if (p == NULL && strcmp(command, "\n") != 0)
	{
		switch_errno = EINVAL;
		return 0;
	}
	else if ( p!=NULL)
	{
		return 0;
	}
	switch_errno = EINVAL;
	return -1;

}

int validatescript(struct comparameter* parameter)
{
	enum script_state {
		start_of_line, // chomp space
		start_of_command,
		start_of_single_line_comment,
		start_of_multi_line_comment,
	};
	uint8_t end_of_comment = 0;
	uint32_t scriptState = start_of_line;
	size_t len = 0;
	FILE* script_file = NULL;
	char* cmd_start = NULL;
	char* cmd_end = NULL;
	char buf[MAXCMD];
	char* ptr = NULL;
	if (!parameter)
	{
		switch_errno = EINVAL;
		return -1;
	}
	if (
		parameter->type1 == com_type_socket &&
		parameter->type2 == com_type_null &&
		parameter->paramType == com_param_type_string &&
		parameter->paramValue.stringValue != NULL
		)
	{
		if (fopen_s(&script_file, parameter->paramValue.stringValue, "r") == 0)
		{

			if (script_file == NULL)
			{
				return switch_errno;
			}
			else
			{
				ptr = fgets(buf, MAXCMD, script_file);
				cmd_start = buf;
				len = strlen(buf);
				while (ptr !=NULL)
				{
					len = strlen(buf);
					if (scriptState == start_of_line)
					{
						if (cmd_start != NULL && *cmd_start == '\n')
						{
							ptr = fgets(buf, MAXCMD, script_file);
							len = strlen(buf);
							scriptState = start_of_line;
							continue;
						}
						if (len > 1 && buf[len - 1] == '\n')
						{
							buf[len - 1] = '\0';
						}
						cmd_start = buf;
						while (*cmd_start == ' ' || *cmd_start == '\t') // Remove leading spaces
						{
							cmd_start++;
						}
						cmd_end = &buf[len - 1];
						while (*cmd_end == ' ' || *cmd_start == '\t') // renmove ending space
						{
							cmd_end--;
						}
						if (*cmd_start == ';')
						{
							// Skip Commend Line
							ptr = fgets(buf, MAXCMD, script_file);
							len = strlen(buf);
							scriptState = start_of_line;
							continue;
						}
						else if (*cmd_start == '#')
						{
							ptr = fgets(buf, MAXCMD, script_file);
							len = strlen(buf);
							scriptState = start_of_line;
							continue;
						}
						else if (*cmd_start == '/' && *(cmd_start + 1) == '/')
						{
							ptr = fgets(buf, MAXCMD, script_file);
							len = strlen(buf);
							scriptState = start_of_line;
							continue;
						}
						else if (*cmd_start == '-' && *(cmd_start + 1) == '-')
						{
							ptr = fgets(buf, MAXCMD, script_file);
							len = strlen(buf);
							scriptState = start_of_line;
							continue;
						}
						else if (*cmd_start == '/' && *(cmd_start + 1) == '*')
						{
							scriptState = start_of_multi_line_comment;
						}
						scriptState = start_of_command;
						
					}
					if (scriptState == start_of_multi_line_comment|| (scriptState == start_of_command && *cmd_start == '/' && *(cmd_start + 1) == '*'))
					{
						scriptState = start_of_multi_line_comment;
						while (end_of_comment == 0)
						{
							if (*cmd_start == '*' && *(cmd_start + 1) == '/')
							{
								scriptState = start_of_line;
								end_of_comment = 1;
								cmd_start++;
								cmd_start++;
								break;
							}
							if (*cmd_start == '\0')
							{
								ptr = fgets(buf, MAXCMD, script_file);
								len = strlen(buf);
								cmd_start = buf;
								continue;
							}
							cmd_start++;
						}
						end_of_comment = 0;
					}
					if (scriptState == start_of_command && validatecommand(cmd_start) == -1)
					{
						switch_errno = EINVAL;
						return -1;
					}
					else if (scriptState == start_of_command)
					{
						scriptState = start_of_line;
						ptr = fgets(buf, MAXCMD, script_file);
						len = strlen(buf);
						cmd_start = buf;
					}
					//handle_cmd(mgmt_data, parameter->data1.socket, buf);
				}
				fclose(script_file);
				return 0;
			}
		}
	}
	return 0;
}

int runscript(struct comparameter* parameter)
{
	enum script_state {
		start_of_line, // chomp space
		start_of_command,
		start_of_single_line_comment,
		start_of_multi_line_comment,
	};
	uint8_t end_of_comment = 0;
	uint32_t scriptState = start_of_line;
	size_t len = 0;
	FILE* script_file = NULL;
	char* cmd_start = NULL;
	char* cmd_end = NULL;
	char buf[MAXCMD];
	char* ptr = NULL;
	if (!parameter)
	{
		switch_errno = EINVAL;
		return -1;
	}
	if (
		parameter->type1 == com_type_socket &&
		parameter->type2 == com_type_null &&
		parameter->paramType == com_param_type_string &&
		parameter->paramValue.stringValue != NULL
		)
	{
		if (validatescript(parameter) == 0)
		{
			if (fopen_s(&script_file, parameter->paramValue.stringValue, "r") == 0)
			{

				if (script_file == NULL)
				{
					return switch_errno;
				}
				else
				{
					ptr = fgets(buf, MAXCMD, script_file);
					cmd_start = buf;
					len = strlen(buf);
					while (ptr != NULL)
					{
						len = strlen(buf);
						if (scriptState == start_of_line)
						{
							if (cmd_start != NULL && *cmd_start == '\n')
							{
								ptr = fgets(buf, MAXCMD, script_file);
								len = strlen(buf);
								scriptState = start_of_line;
								continue;
							}
							if (len > 1 && buf[len - 1] == '\n')
							{
								buf[len - 1] = '\0';
							}
							cmd_start = buf;
							while (*cmd_start == ' ' || *cmd_start == '\t') // Remove leading spaces
							{
								cmd_start++;
							}
							cmd_end = &buf[len - 1];
							while (*cmd_end == ' ' || *cmd_start == '\t') // renmove ending space
							{
								cmd_end--;
							}
							if (*cmd_start == ';')
							{
								// Skip Commend Line
								ptr = fgets(buf, MAXCMD, script_file);
								len = strlen(buf);
								scriptState = start_of_line;
								continue;
							}
							else if (*cmd_start == '#')
							{
								ptr = fgets(buf, MAXCMD, script_file);
								len = strlen(buf);
								scriptState = start_of_line;
								continue;
							}
							else if (*cmd_start == '/' && *(cmd_start + 1) == '/')
							{
								ptr = fgets(buf, MAXCMD, script_file);
								len = strlen(buf);
								scriptState = start_of_line;
								continue;
							}
							else if (*cmd_start == '-' && *(cmd_start + 1) == '-')
							{
								ptr = fgets(buf, MAXCMD, script_file);
								len = strlen(buf);
								scriptState = start_of_line;
								continue;
							}
							else if (*cmd_start == '/' && *(cmd_start + 1) == '*')
							{
								scriptState = start_of_multi_line_comment;
							}
							scriptState = start_of_command;

						}
						if (scriptState == start_of_multi_line_comment || (scriptState == start_of_command && *cmd_start == '/' && *(cmd_start + 1) == '*'))
						{
							scriptState = start_of_multi_line_comment;
							while (end_of_comment == 0)
							{
								if (*cmd_start == '*' && *(cmd_start + 1) == '/')
								{
									scriptState = start_of_line;
									end_of_comment = 1;
									cmd_start++;
									cmd_start++;
									break;
								}
								if (*cmd_start == '\0')
								{
									ptr = fgets(buf, MAXCMD, script_file);
									len = strlen(buf);
									cmd_start = buf;
									continue;
								}
								cmd_start++;
							}
							end_of_comment = 0;
						}
						if (scriptState == start_of_command && validatecommand(cmd_start) == -1)
						{
							switch_errno = EINVAL;
							return -1;
						}
						else if (scriptState == start_of_command)
						{
							scriptState = start_of_line;
							ptr = fgets(buf, MAXCMD, script_file);
							len = strlen(buf);
							cmd_start = buf;
						}
						handle_cmd(mgmt_data, parameter->data1.socket, buf);
					}
					fclose(script_file);
					return 0;
				}
			}
		}
		else
		{
			fprintf(stderr, "Failed to validate the script - will not load\n");
			return -1;
		}
	}
	return 0;
}

int help(struct comparameter* parameter)
{
	struct comlist* p=NULL;
	size_t n = 0;
	char* tmpBuff = NULL;
	size_t length = 0;
	if (!parameter)
	{
		switch_errno = EINVAL;
		return -1;
	}
	if (parameter->type1 == com_type_file && parameter->data1.file_descriptor == NULL)
	{
		switch_errno = EINVAL;
		return -1;
	}
	if (parameter->type1 == com_type_file && parameter->paramType==com_param_type_string&& parameter->paramValue.stringValue!=NULL)
	{
		printoutc(parameter->data1.file_descriptor, "%-18s %-15s %s", "COMMAND PATH", "SYNTAX", "HELP");
		printoutc(parameter->data1.file_descriptor, "%-18s %-15s %s", "------------", "--------------", "------------");
		for (p = clh; p != NULL; p = p->next)
		{
			if (strncmp(p->path, parameter->paramValue.stringValue, n) == 0)
			{
				printoutc(parameter->data1.file_descriptor, "%-18s %-15s %s", p->path, p->syntax, p->help);
			}
		}
	}
	else if (parameter->type1 == com_type_socket && parameter->data1.socket!=INVALID_SOCKET && parameter->paramType == com_param_type_string && parameter->paramValue.stringValue != NULL)
	{
		
		length = asprintf(&tmpBuff, "%-18s %-15s %s\n", "COMMAND PATH", "SYNTAX", "HELP");
		if (length>0 && tmpBuff)
		{
			send(parameter->data1.socket, tmpBuff, (int)length, 0);
			free(tmpBuff);
		}
		else
		{
			return -1;
		}
		length = asprintf(&tmpBuff, "%-18s %-15s %s\n", "------------", "--------------", "------------");
		if (length > 0 && tmpBuff)
		{
			send(parameter->data1.socket, tmpBuff, (int)length, 0);
			free(tmpBuff);
		}
		else
		{
			return -1;
		}
		
		
		for (p = clh; p != NULL; p = p->next)
		{
			if (strncmp(p->path, parameter->paramValue.stringValue, n) == 0)
			{
				length = asprintf(&tmpBuff, "%-18s %-15s %s\n", p->path, p->syntax, p->help);
				if (length > 0 && tmpBuff)
				{
					send(parameter->data1.socket, tmpBuff, (int)length, 0);
					free(tmpBuff);
				}
				else
				{
					return -1;
				}
			}
		}
	}
	else if (parameter->type1 == com_type_memstream && parameter->data1.mem_stream != NULL && parameter->paramType == com_param_type_string && parameter->paramValue.stringValue != NULL)
	{
		length = asprintf(&tmpBuff, "%-18s %-15s %s\n", "COMMAND PATH", "SYNTAX", "HELP");
		if (length > 0 && tmpBuff)
		{
			write_memorystream(parameter->data1.mem_stream, tmpBuff, length);
			free(tmpBuff);
		}
		else
		{
			return -1;
		}
		length = asprintf(&tmpBuff, "%-18s %-15s %s\n", "------------", "--------------", "------------");
		if (length > 0 && tmpBuff)
		{
			write_memorystream(parameter->data1.mem_stream, tmpBuff, length);
			free(tmpBuff);
		}
		else
		{
			return -1;
		}
		for (p = clh; p != NULL; p = p->next)
		{
			if (strncmp(p->path, parameter->paramValue.stringValue, n) == 0)
			{
				length = asprintf(&tmpBuff, "%-18s %-15s %s\n", p->path, p->syntax, p->help);
				if (length > 0 && tmpBuff)
				{
					write_memorystream(parameter->data1.mem_stream, tmpBuff, length);
					free(tmpBuff);
				}
				else
				{
					return -1;
				}
			}
		}
	}
	return 0;
}

int handle_cmd(int type, SOCKET socketDescriptor, char* input_buffer)
{
	struct comlist* p;
	int rv = ENOSYS;
	char* outbuf=NULL;
	size_t * outbufsize=0;
	struct _memory_file* memStream = NULL;
	struct comparameter parameter;
	char* sprintBuff = NULL;
	if (!input_buffer)
	{
		switch_errno = EINVAL;
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
			if (p == NULL && strcmp(input_buffer,"\n")!=0)
			{
				switch_errno = EINVAL;
				rv = 1;
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
							write_memorystream(memStream, "0000 DATA END WITH '.'\n", strlen("0000 DATA END WITH '.'\n"));
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
									rv = p->doit(&parameter); 
								break;
							}
							write_memorystream(memStream, ".\n", strlen(".\n"));
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
					write_memorystream(memStream, "0000 DATA END WITH '.'\n", strlen("0000 DATA END WITH '.'\n"));
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
					write_memorystream(memStream, ".\n", strlen(".\n"));
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
				write_memorystream(memStream, "1000 Success\n", strlen("1000 Success\n"));
			}
			else if (rv > 0)
			{
				strerror_s(errorbuff, sizeof(errorbuff), switch_errno);
				if (asprintf(&sprintBuff, "1%03d %s", rv, errorbuff)!=-1)
				{

					write_memorystream(memStream, sprintBuff, strlen(sprintBuff));
				}

				//write_memorystream(f, "1000 Success", strlen("1000 Success"));
			}

			if (socketDescriptor >= 0)
			{
				get_buffer(memStream);
				send(socketDescriptor, outbuf, (int)*outbufsize, 0);
			}
			close_memorystream(memStream);
		}
		else
		{
			fprintf(stderr,"Failed to open the memory stream\n");
			switch_errno = ENOMEM;
			return -1;
		}
	}
	return rv;
}

void loadrcfile(void)
{
	size_t size;
	struct comparameter parameter;
	if (rcfile != NULL)
	{
		parameter.type1 = com_type_socket;
		parameter.data1.socket = INVALID_SOCKET;
		parameter.type2 = com_type_null;
		parameter.paramType = com_param_type_string;
		parameter.paramValue.stringValue = rcfile;
		runscript(&parameter);
	}
	else
	{
		char path[MAX_PATH];
		char home[MAX_PATH];
		getenv_s(&size, home, sizeof(home), "USERPROFILE");
		snprintf(path, MAX_PATH, "%s\\.winvde2\\winvde_switch.rc", home);
		if (_access(path, R_OK) == 0)
		{
			parameter.type1 = com_type_socket;
			parameter.data1.socket = INVALID_SOCKET;
			parameter.type2 = com_type_null;
			parameter.paramType = com_param_type_string;
			parameter.paramValue.stringValue = path;
			runscript(&parameter);
		}
		else
		{
			if (_access(STDRCFILE, R_OK) == 0)
			{
				parameter.type1 = com_type_socket;
				parameter.data1.socket = INVALID_SOCKET;
				parameter.type2 = com_type_null;
				parameter.paramType = com_param_type_string;
				parameter.paramValue.stringValue = STDRCFILE;
				runscript(&parameter);
			}
		}
	}
}


void RunDaemon()
{

}

void save_pidfile()
{
	if (pidfile[0] != '/')
	{
		strncat_s(pidfile_path, MAX_PATH,pidfile, MAX_PATH - strlen(pidfile_path) - 1);
	}
	else
		strncpy_s(pidfile_path, MAX_PATH,pidfile, MAX_PATH - 1);

	
	FILE* fileHandle = NULL;
	switch_errno = fopen_s(&fileHandle, pidfile_path, "w");
	if (switch_errno != 0 || fileHandle==NULL) {
		strerror_s(errorbuff,sizeof(errorbuff),switch_errno);
		printlog(LOG_ERR, "Error in pidfile creation: %s",errorbuff);
		exit(1);
	}

	if (fprintf(fileHandle, "%ld\n", (long int)_getpid()) <= 0) {
		printlog(LOG_ERR, "Error in writing pidfile");
		exit(1);
	}
	fclose(fileHandle);
}

void mgmtnewfd(SOCKET new)
{
	char buf[MAXCMD];
	DWORD one = 1;
	if (ioctlsocket(new, FIONBIO, &one)==SOCKET_ERROR)
	{
		strerror_s(errorbuff, sizeof(errorbuff), switch_errno);
		printlog(LOG_WARNING, "mgmt fcntl - setting O_NONBLOCK: %s - %d\n", errorbuff, WSAGetLastError());
		closesocket(new);
		return;
	}

	add_fd(new, mgmt_data, mgmgt_module.module_tag, NULL);
	eventout(MGMTPORTNEW, new);
	snprintf(buf, MAXCMD, header, PACKAGE_VERSION);
	send(new, buf, strlength(buf),0);
	send(new, prompt, strlength(prompt),0);
}

#ifdef DEBUGOPT
void sighupmgmt(int signo)
{
	EVENTOUT(MGMTSIGHUP, signo);
}
#endif

void setmgmtperm(char* path)
{
	//chmod(path, mgmt_mode);
	//chown(path, -1, mgmt_group);
}