#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#include <libwinvde.h>

#include "openclosepidfile.h"

#pragma comment(lib,"winvde.lib")
#pragma comment(lib,"ws2_32.lib")

WINVDECONN * connection1 = NULL;
WINVDECONN * connection2 = NULL;

WINVDESTREAM* winvdestream;

#define ETH_ALEN 6
#define ETH_HDRLEN (ETH_ALEN+ETH_ALEN+2)

#define VDE_IP_LOG_GROUP "winvdeplug_iplog"
#define DEFAULT_DESCRIPTION "winvdeplug:"
#define MAX_HOSTNAME 255
#define MAX_ERROR 1024
char hostname[MAX_HOSTNAME];
unsigned char buffin[WINVDE_ETHBUFSIZE];
char errbuff[MAX_ERROR];

BOOL DoDaemonize = FALSE;
BOOL DoPidFile = FALSE;
BOOL DoLogging = FALSE;
BOOL DoLogIp = FALSE;
BOOL DoGroup1 = FALSE;
BOOL DoGroup2 = FALSE;
BOOL DoMode1 = FALSE;
BOOL DoMode2 = FALSE;
BOOL DoPort1 = FALSE;
BOOL DoPort2 = FALSE;
BOOL DoDescription = FALSE;
BOOL DoWinVdeUrl1 = FALSE;
BOOL DoWinVdeUrl2 = FALSE;
BOOL DoCommand = FALSE;

char * otherGlobal  =NULL;
char * pidfile = NULL;
char* group1 = NULL;
char* group2 = NULL;
char* mode1 = NULL;
char* mode2 = NULL;
char* port1 = NULL;
char* port2 = NULL;
char* description = NULL;
char* command = NULL;
char * winvde_url1 = NULL;
char * winvde_url2 = NULL;

void winvde_error(void* opaque, int type, char* format, ...);

size_t winvde_plugrecv(void * opaque, void * buf, size_t count);
void winvde_plugcleanup(void);
int plug2stream(void);
int plug2cmd(char ** argv);
int plug2plug();
void netusage_and_exit();
void usage_and_exit(char* progname);
void GetOptions(const int argc, const char ** argv);
void VerboseOptions(const int argc, const char** argv);

int main(const int argc, const char ** argv)
{
    char* progname;
    struct winvde_open_args open_args = { .port = 0,.group = NULL,.mode = 0700 };
    struct winvde_open_args open_args2 = { .port = 0,.group = NULL,.mode = 0700 };
    WSADATA wsadata;
    if(argc == 0 || argv == NULL)
    {
        fprintf(stderr, "Invalid Exe\n");
        exit(-1);
    }
    if (argc >= 1)
    {
        // Set program name
        progname = argv[0];

        // Need windows sockets set up for everything
        memset(&wsadata, 0, sizeof(WSADATA));
        if (WSAStartup(MAKEWORD(2, 2), &wsadata) != ERROR_SUCCESS)
        {
            fprintf(stderr, "Failed to start windows sockets: %d\n", WSAGetLastError());
            exit(0xFFFFFFFF);
        }

        // Need host name
        gethostname(hostname, MAX_HOSTNAME);

        // This should not happen on windows
        if (argv[0][0] == '-')
        {
            netusage_and_exit();
        }

        VerboseOptions(argc, argv);

        GetOptions(argc, argv);

        if (DoDaemonize)
        {
            fprintf(stdout, "WARNING: Daemonize not supported currently\n");
        }

        if (DoPidFile)
        {
            fprintf(stdout, "WARNING: PID not supported currently\n");
            //openclosepidfile(pidfile);
        }

        atexit(winvde_plugcleanup);
        
        setsighandlers(winvde_plugcleanup);
        
        if (DoWinVdeUrl1)
        {
            connection1 = winvde_open_real(winvde_url1, description, LIBWINVDE_PLUG_VERSION,(struct winvde_open_args*)&open_args);
            if (connection1 == NULL)
            {
                strerror_s(errbuff,sizeof(errbuff),errno);
                fprintf(stderr, "winvde_open %s: %s\n", winvde_url1 && *winvde_url1 ? winvde_url1 : "default switch", errbuff);
                WSACleanup();
                exit(0xFFFFFFFF);
            }

            if (DoWinVdeUrl2 && winvde_url2 != NULL)
            {
                connection2 = winvde_open_real(winvde_url2, description, LIBWINVDE_PLUG_VERSION,(struct winvde_open_args*) (& open_args2));
                if (connection2 != NULL)
                {
                    strerror_s(errbuff, sizeof(errbuff), errno);
                    fprintf(stderr, "winvde_open %s: %s\n", winvde_url2 && *winvde_url2 ? winvde_url2 : "default switch", errbuff);
                    WSACleanup();
                    exit(0xFFFFFFFF);
                }
                return plug2plug();
            }
            else if (command != NULL)
            {
                return plug2cmd(command);
            }
            else
            {
                return plug2stream();
            }
        }

        // Shutdown sockets
        WSACleanup();
    }
    else
    {
        fprintf(stderr, "Sanity Check! Something wierd happened\n");
        exit(-1);
    }

    return 0;
}


void winvde_error(void* opaque, int type, char* format, ...)
{

    va_list args;
    fprintf(stderr, "%s: Packet length error: %d\n", hostname, GetLastError());
    va_start(args,format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
}

size_t winvde_plugrecv(void* opaque, void* buff, size_t count)
{
    WINVDECONN* conn = (WINVDECONN*)opaque;
    if (opaque != NULL && buff != NULL)
    {
        return winvde_send(conn, (char*)buff,count,0);
    }
    else
    {
        errno = EINVAL;
        return -1;
    }
}

void winvde_plugcleanup(void)
{
    if (connection1)
    {
        winvde_close(connection1);
    }
    
    if (connection2 != NULL)
    {
        winvde_close(connection2);
    }
    if (winvdestream != NULL)
    {
        winvdestream_close(winvdestream);
    }
    openclosepidfile(NULL);
}

int plug2stream(void)
{
    size_t nx;
    struct pollfd pollv[] =
    {
        {stdin,POLLIN | POLLHUP},
        {winvde_datafiledescriptor(connection1),POLLIN | POLLHUP},
        {winvde_ctlfiledescriptor(connection1),POLLIN | POLLHUP},
    };
    winvdestream = winvdestream_open(connection1, stdout, winvde_plugrecv, winvde_error);
    for (;;)
    {
        WSAPoll(pollv,3,-1);
        if ((pollv[0].revents | pollv[1].revents | pollv[2].revents) & POLLHUP || (pollv[2].revents & POLLIN))
        {
            break;
        }
        if ((pollv[0].revents & POLLIN))
        {
            nx = read(stdin, buffin, sizeof(buffin));
            if (nx == 0)
            {
                break;
            }
            winvdestream_recv(winvdestream, buffin, nx);
        }
        if (pollv[1].revents & POLLIN)
        {
            nx = winvde_recv(connection1, buffin, WINVDE_ETHBUFSIZE, 0);
            if (nx >= ETH_HDRLEN)
            {
                winvdestream_send(winvdestream, buffin, nx);
            }
            else if ( nx<0)
            {
                perror("winvdeplug: recvfrom\n");
            }
        }

    }
    return 0;
}

int plug2cmd(char** argv)
{
    STARTUPINFO startupInfo;
    PROCESS_INFORMATION processInfo;
    
    SECURITY_ATTRIBUTES saAttr;

    HANDLE g_hChildStd_IN_Rd = NULL;
    HANDLE g_hChildStd_IN_Wr = NULL;
    HANDLE g_hChildStd_OUT_Rd = NULL;
    HANDLE g_hChildStd_OUT_Wr = NULL;


    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0))
    {
        fprintf(stderr, "Failed to create the pipe for stdout: %d\n", GetLastError());
        return -1;
    }

    if (!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))
    {
        CloseHandle(g_hChildStd_OUT_Rd);
        g_hChildStd_OUT_Rd = NULL;
        fprintf(stderr,"Failed to set flagInherit on Stdout: %d\n", GetLastError());
        return -1;
    }

    if (!CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0))
    {
        CloseHandle(g_hChildStd_OUT_Rd);
        g_hChildStd_OUT_Rd = NULL;
        fprintf(stderr, "Failed to create stdInPipe: %d\n", GetLastError());
    }

    if (!SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0))
    {
        CloseHandle(g_hChildStd_OUT_Rd);
        g_hChildStd_OUT_Rd = NULL;
        CloseHandle(g_hChildStd_IN_Wr);
        g_hChildStd_IN_Wr = NULL;
        fprintf(stderr,"Failed to set flagInherit on Stdin: %d\n",GetLastError());
    }
        


    memset(&startupInfo, 0, sizeof(STARTUPINFO));
    memset(&processInfo, 0, sizeof(PROCESS_INFORMATION));

    startupInfo.cb = sizeof(STARTUPINFO);
    startupInfo.cbReserved2 = 0;
    startupInfo.lpReserved = 0;
    startupInfo.lpReserved2 = 0;
    startupInfo.hStdInput = STARTF_USESTDHANDLES;
    startupInfo.hStdError = g_hChildStd_OUT_Wr;
    startupInfo.hStdOutput = g_hChildStd_OUT_Wr;
    startupInfo.hStdInput = g_hChildStd_IN_Rd;

    if (!CreateProcessA(NULL, argv[0], NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL, NULL, &startupInfo, &processInfo))
    {
        fprintf(stderr, "Failed to create the process: %s %d\n", argv[0], GetLastError());
        CloseHandle(g_hChildStd_OUT_Rd);
        g_hChildStd_OUT_Rd = NULL;
        CloseHandle(g_hChildStd_IN_Wr);
        g_hChildStd_IN_Wr = NULL;
        return -1;
    }

    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);

    // Close handles to the stdin and stdout pipes no longer needed by the child process.
    // If they are not explicitly closed, there is no way to recognize that the child process has ended.

    CloseHandle(g_hChildStd_OUT_Wr);
    CloseHandle(g_hChildStd_IN_Rd);

    return plug2stream();
}

int plug2plug()
{
    size_t nx = 0;
    struct pollfd pollv[] =
    {
        {winvde_datafiledescriptor(connection1),POLLIN | POLLHUP },
        {winvde_datafiledescriptor(connection2),POLLIN|POLLHUP},
        {winvde_ctlfiledescriptor (connection1), POLLIN | POLLHUP},
        {winvde_ctlfiledescriptor(connection2),POLLIN|POLLHUP},

    };
    for (;;)
    {
        WSAPoll(pollv, 4, -1);
        if ((pollv[0].revents | pollv[1].revents | pollv[2].revents | pollv[3].revents) & POLLHUP ||
            (pollv[2].revents | pollv[3].revents) & POLLIN)
        {
            break;
        }
        if (pollv[0].revents &POLLIN)
        {
            nx = winvde_recv(connection1, buffin, WINVDE_ETHBUFSIZE, 0);
            if (nx > ETH_HDRLEN)
            {
                winvde_send(connection2,buffin,nx,0);
            }
            else if (nx < 0)
            {
                break;
            }
        }
        if (pollv[1].revents & POLLIN)
        {
            nx = winvde_recv(connection2, buffin, WINVDE_ETHBUFSIZE, 0);
            if (nx > ETH_HDRLEN)
            {
                winvde_send(connection1, buffin, nx, 0);
            }
            else if (nx < 0)
            {
                break;
            }

        }
    }
    return 0;
}

void netusage_and_exit()
{
    fprintf(stderr, "This is a Virtual Distributed Ethernet (vde) tunnel broker. \n");
    fprintf(stderr, "This is not a login shell, only vde_plug can be executed\n\n");
    exit(-1);
}

void usage_and_exit(char * progname)
{
    if (progname == NULL)
    {
        progname = (char*)"Unspecified Program";
    }
    fprintf(stderr, "Usage:\n"
        " %s [ OPTIONS ] \n"
        " %s [ OPTIONS ] winvde_plug_url \n"
        " %s [ OPTIONS ] winvde_plug_url winvde_plug_url \n"
        " %s [ OPTIONS ] = cmd [ args... ] \n"
        " %s [ OPTIONS ] winvde_plug_url = cmd [args...] \n"
        " an ommitted or empty\"\" winvde_plug_url refersto the default winvde service \n"
        " OPTIONS: \n"
        " -d daemonize | --daemonize not supported on windows\n"
        " -p <PIDFILE> | --pid-file <PIDFILE> \n"
        " -l | --log generate log files\n"
        " -L | --log-ip generate ip in log files\n"
        " -g <GROUP> | --group <GROUP> set group ownership for connection1 \n"
        " -G <GROUP> | --group2 <GROUP> set group ownership for connection2 \n"
        " -m <MODE> | --mode <MODE> set the socket protection mode for connection1 \n"
        " -M <MODE> | --mode2 <MODE> set the socket protection mode for connection2 \n"
        " -x|--port <PORT1> -X|--port2 <PORT> set the switch port NOTE: Obsolete use [port=] suffix"
        " -D <DESCR> --descr <DESCR> set the description of the connection\n"
        "\n", progname, progname, progname, progname, progname);
    exit(0xFFFFFFFF);
}


void GetOptions(const int argc, const char** argv)
{
    char* parameter1 = NULL;
    char* parameter2 = NULL;
    
    uint32_t index = 0;
    BOOL lastdash = FALSE;
    BOOL equals = FALSE;
    size_t compose_length = 0;
    uint32_t arg_count = 0;
    uint32_t first_arg = 0;
    if (argc <= 0 || argv == NULL)
    {
        return;
    }
    // skip first argument which should be application name
    for (index = 1; index < (uint32_t)argc; index++)
    {
        parameter1 = NULL;
        parameter2 = NULL;
        if (argv[index] != NULL)
        {
            if (index < argc - 1)
            {
                parameter1 = argv[index + 1];
            }
            if (index < argc - 2)
            {
                parameter2 = argv[index + 2];
            }
            if (!lastdash && !equals)
            {
                if (strcmp(argv[index], "-d") == 0 || strcmp(argv[index], "--daemonize") == 0)
                {
                    if (!DoDaemonize)
                    {
                        DoDaemonize = TRUE;
                    }
                    else
                    {
                        fprintf(stderr, "Daemonize must only be specified once\n");
                        exit(0xFFFFFFFF);
                    }
                }
                else if (strcmp(argv[index], "-p") == 0 || strcmp(argv[index], "--pid-file") == 0)
                {
                    if (!DoPidFile)
                    {
                        if (parameter1 != NULL)
                        {
                            pidfile = parameter1;
                            index++;
                        }
                        else
                        {
                            fprintf(stderr, "PidFile requires an argument\n");
                            exit(0xFFFFFFFF);
                        }
                    }
                    else
                    {
                        fprintf(stderr, "PidFile must only be specified once\n");
                        exit(0xFFFFFFFF);
                    }
                }
                else if (strcmp(argv[index], "-l") == 0 || strcmp(argv[index], "--log") == 0)
                {
                    if (!DoLogging)
                    {
                        DoLogging = TRUE;
                    }
                    else
                    {
                        fprintf(stderr, "Logging must only be specified once\n");
                        exit(0xFFFFFFFF);
                    }
                }
                else if (strcmp(argv[index], "-L") == 0 || strcmp(argv[index], "--log-ip") == 0)
                {
                    if (!DoLogIp)
                    {
                        DoLogIp = TRUE;
                    }
                    else
                    {
                        fprintf(stderr, "LogIp must only be specified once\n");
                        exit(0xFFFFFFFF);
                    }
                }
                else if (strcmp(argv[index], "-g") == 0 || strcmp(argv[index], "--group1") == 0)
                {
                    if (!DoGroup1)
                    {
                        DoGroup1 = TRUE;
                        if (parameter1 != NULL)
                        {
                            group1 = parameter1;
                            index++;
                        }
                        else
                        {
                            fprintf(stderr, "Group1 requires an argument\n");
                            exit(0xFFFFFFFF);
                        }
                    }
                    else
                    {
                        fprintf(stderr, "First Group must only be specified once\n");
                        exit(0xFFFFFFFF);
                    }
                }
                else if (strcmp(argv[index], "-G") == 0 || strcmp(argv[index], "--group2") == 0)
                {
                    if (!DoGroup2)
                    {
                        DoGroup2 = TRUE;
                        if (parameter1 != NULL)
                        {
                            group2 = parameter1;
                            index++;
                        }
                        else
                        {
                            fprintf(stderr, "Group2 requires an argument\n");
                            exit(0xFFFFFFFF);
                        }
                    }
                    else
                    {
                        fprintf(stderr, "Second Group must only be specified once\n");
                        exit(0xFFFFFFFF);
                    }
                }
                else if (strcmp(argv[index], "-m") == 0 || strcmp(argv[index], "--mode1") == 0)
                {
                    if (!DoMode1)
                    {
                        DoMode1 = TRUE;
                        if (parameter1 != NULL)
                        {
                            mode1 = parameter1;
                            index++;
                        }
                        else
                        {
                            fprintf(stderr, "mode1 requires an argument\n");
                            exit(0xFFFFFFFF);
                        }
                    }
                    else
                    {
                        fprintf(stderr, "First Mode must only be specified once\n");
                        exit(0xFFFFFFFF);
                    }
                }
                else if (strcmp(argv[index], "-M") == 0 || strcmp(argv[index], "--mode2") == 0)
                {
                    if (!DoMode2)
                    {
                        DoMode2 = TRUE;
                        if (parameter1 != NULL)
                        {
                            mode2 = parameter1;
                            index++;
                        }
                        else
                        {
                            fprintf(stderr, "mode12 requires an argument\n");
                            exit(0xFFFFFFFF);
                        }
                    }
                    else
                    {
                        fprintf(stderr, "Second Node must only be specified once\n");
                        exit(0xFFFFFFFF);
                    }
                }
                else if (strcmp(argv[index], "-x") == 0 || strcmp(argv[index], "--port1") == 0)
                {
                    if (!DoPort1)
                    {
                        DoPort1 = TRUE;
                        if (parameter1 != NULL)
                        {
                            port1 = parameter1;
                            index++;
                        }
                        else
                        {
                            fprintf(stderr, "port1 requires an argument\n");
                            exit(0xFFFFFFFF);
                        }
                    }
                    else
                    {
                        fprintf(stderr, "First Port must only be specified once\n");
                        exit(0xFFFFFFFF);
                    }
                }
                else if (strcmp(argv[index], "-X") == 0 || strcmp(argv[index], "--port2") == 0)
                {
                    if (!DoPort2)
                    {
                        DoPort2 = TRUE;
                        if (parameter1 != NULL)
                        {
                            port2 = parameter1;
                            index++;
                        }
                        else
                        {
                            fprintf(stderr, "Port2 requires an argument\n");
                            exit(0xFFFFFFFF);
                        }
                    }
                    else
                    {
                        fprintf(stderr, "Second Port must only be specified once\n");
                        exit(0xFFFFFFFF);
                    }
                }
                else if (strcmp(argv[index], "-D") == 0 || strcmp(argv[index], "--descr") == 0)
                {
                    if (!DoDescription)
                    {
                        DoDescription = TRUE;
                        if (parameter1 != NULL)
                        {
                            description = parameter1;
                            index++;
                        }
                        else
                        {
                            fprintf(stderr, "Description requires an argument\n");
                            exit(0xFFFFFFFF);
                        }
                    }
                    else
                    {
                        fprintf(stderr, "Description must only be specified once\n");
                        exit(0xFFFFFFFF);
                    }
                }
                else if (strcmp(argv[index], "-") == 0)
                {
                    // last dash argument
                    if (!lastdash)
                    {
                        lastdash = TRUE;
                    }
                    else
                    {
                        fprintf(stderr, "Last Dash triggered twice\n");
                        exit(0xFFFFFFFF);
                    }
                }
                else if (strcmp(argv[index],"="))
                {
                    if (!equals)
                    {
                        equals = TRUE;
                        first_arg = index + 1;
                    }
                    else
                    {
                        fprintf(stderr, "Equals triggered twice\n");
                        exit(0xFFFFFFFF);
                    }
                }
            }
            else if ( strcmp(argv[index],"=")==0)
            {
                // This is a command line
                // Grab all the rest of the arguments and compose a command line string
                if (!equals)
                {
                    equals = TRUE;
                    first_arg = index + 1;
                }
                else
                {
                    fprintf(stderr, "Equals triggered twice\n");
                    exit(0xFFFFFFFF);
                }
            }
            else
            {
                if (equals)
                {
                    compose_length += strlen(argv[index])+1;
                    arg_count++;
                }
                else
                {
                    if (DoWinVdeUrl1 == FALSE)
                    {
                        DoWinVdeUrl1 = TRUE;
                        winvde_url1 = argv[index];
                    }
                    else if (DoWinVdeUrl2 == FALSE)
                    {
                        DoWinVdeUrl2 = TRUE;
                        winvde_url2 = argv[index];
                    }
                    else
                    {
                        fprintf("Invalid Arguments: %s\n", argv[index]);
                        exit(0xFFFFFFFF);
                    }
                }
            }
        }
        else
        {
            fprintf(stderr, "Argument should not be null\n");
            exit(0xFFFFFFFF);
        }
    }
    if (first_arg > 0 && equals && compose_length>0)
    {
        command = (char *)malloc(compose_length+1);
        if (command != NULL)
        {
            strcpy_s(command, compose_length, "");
            for (index = first_arg; index < first_arg + arg_count;index++)
            {
                strcat_s(command, compose_length+1, argv[index]);
                if(index< first_arg + arg_count)
                {
                    strcat_s(command, compose_length + 1, " ");
                } // dont append extra spaces
                
            }
            DoCommand = TRUE;
        }
        else
        {
            fprintf(stderr, "Failed to allocate memory for command\n");
            exit(0xFFFFFFFF);
        }
    }
}

void VerboseOptions(const int argc, const char ** argv)
{
    uint32_t index = 0;
    if (argc <= 0 || argv == NULL)
    {
        return;
    }
    for (index = 0; index < (uint32_t)argc; index++)
    {
        if (argv[index] != NULL)
        {
            fprintf(stdout, "--argument: %d %s\n", index, argv[index]);
        }
        else
        {
            fprintf(stdout, "--argument: %d %s\n", index, "(NULL)");
        }
    }
}