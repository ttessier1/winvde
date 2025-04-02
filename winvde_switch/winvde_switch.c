#define WIN32_LEAN_AND_MEAN
// standard includes
#include <windows.h>
#include <winmeta.h>
#include <winsock2.h>
#include <stdlib.h>
#include <stdint.h>
#include <process.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>
#include <conio.h>

// custom includes
#include "winvde_mgmt.h"
#include "winvde_datasock.h"
#include "winvde_module.h"
#include "winvde_switch.h"
#include "winvde_hash.h"
#include "winvde_port.h"
#include "winvde_loglevel.h"
#include "winvde_getopt.h"
#include "winvde_descriptor.h"
#include "winvde_output.h"
#include "winvde_qtimer.h"

#if defined(HAVE_TUNTAP)
#include "winvde_tuntap.h"
#endif

#include "version.h"

// add libraries
#pragma comment (lib,"ws2_32.lib")
#pragma comment(lib, "ntdll.lib")
#pragma comment(lib,"Iphlpapi.lib")



#define PRIOFLAG 0x80
#define ISPRIO(X) ((X) & PRIOFLAG)


// global variables
unsigned char switchmac[ETH_ALEN];

char errorbuff[1024];
int hash_size = INIT_HASH_SIZE;
static int numports = INIT_NUMPORTS;
const char* prog;

static uint64_t xrand48 = 0;
static uint64_t xx = 0;
#define BUFFER_SIZE 1024
char std_input_buffer[BUFFER_SIZE];
int std_input_pos = 0;

// function declarations
void srand48(uint64_t seed);
uint64_t lrand48();
int gettimeofday(struct timeval * tv);
void SetMacAddress();
void SetSignalHandlers();
void StartModules();
struct option* optcpy(struct option* tgt, struct option* src, int n, int tag);
int parse_globopt(int c, char* optarg);
void ParseArguments(const int argc, const char ** argv);
void StartTunTap(void);
void sig_handler(int sig);
void FileCleanUp();
void CleanUp();
void Usage();
void Version();
void main_loop();

#if !defined(TEST_QTIMER) && !defined(TEST_BITARRAY) && !defined(TEST_PATH_FUNCTIONS)
int main(const int argc, const char ** argv)
{
    WSADATA wsadata;
    if (argc <= 0)
    {
        fprintf(stderr, "Invalid EXE\n");
        exit(0xFFFFFFFF);
    }

    prog = (char*)argv[0];

    memset(&wsadata,0,sizeof(WSADATA));

    if (WSAStartup(MAKEWORD(2, 2), &wsadata) == SOCKET_ERROR)
    {
        fprintf(stderr, "Failed to start up sockets\n");
        exit(0xFFFFFFF);
    }

    SetMacAddress();

    fprintf(stdout, "Ethernet Switch Mac: %02x:%02x:%02x:%02x:%02x:%02x\n", switchmac[0], switchmac[1], switchmac[2], switchmac[3], switchmac[4], switchmac[5] );

    SetSignalHandlers();

    fprintf(stdout, "Signal handlers set\n");

    StartModules();

    fprintf(stdout, "Modules Started\n");

    ParseArguments(argc,argv);

    fprintf(stdout, "Arguments Parsed\n");

    atexit(CleanUp);

    // Needed before hash functions
    qtimer_init();
    
    fprintf(stdout, "Timer Init\n");

    HashInit(hash_size);

    fprintf(stdout, "Hash Init\n");
#ifdef FSTP
    fst_init(numports);
#endif

    port_init(numports);

    fprintf(stdout, "Port Init\n");

    if (init_mods() == 0)
    {
        fprintf(stdout, "Mods Init\n");

        loadrcfile();

        fprintf(stdout, "Load RcFile\n");

        main_loop();

        fprintf(stdout, "Main Loop\n");
    }
#if defined(HAVE_TUNTAP)
    StopTunTap();
#endif
    WSACleanup();
}

#endif

void SetMacAddress()
{
    struct timeval v;
    unsigned long long val;
    int index;

    gettimeofday(&v);
    srand48(v.tv_sec ^ v.tv_usec ^ _getpid());
    for (index = 0, val = lrand48(); index < 4; index++, val >>= 8)
    {
        switchmac[index + 2] = (char)val&0xFF;
    }
    switchmac[0] = 0;
    switchmac[1] = 0xff;

}

int gettimeofday(struct timeval* tv)
{
    FILETIME lpFileTime;
    ULARGE_INTEGER timeValue64;
    LONGLONG total;
    const ULONGLONG epoch_offset = 11644473600000000ULL;
    if (!tv)
    {
        switch_errno = EINVAL;
        return -1;
    }
#if WIN32_WINNT >= _WIN32_WINNT_WIN8
    GetSystemTimePreciseAsFileTime(&lpFileTime);
#else
    GetSystemTimeAsFileTime(&lpFileTime);
#endif

    timeValue64.HighPart = lpFileTime.dwHighDateTime;
    timeValue64.LowPart = lpFileTime.dwLowDateTime;

    total = timeValue64.QuadPart / 10 - epoch_offset;
    tv->tv_sec = (long)(total / 1000000ULL);
    tv->tv_usec = (long)(total / 1000000ULL);
    return 0;
}

void srand48(uint64_t seed)
{
    xrand48 = seed;
}

uint64_t lrand48()
{
    static uint64_t a = 0x5DEECE66D;
    static uint64_t c = 0xB;
    xx++;
    return (a * xrand48 * xx + c) % 0xFFFFFFFFFFFF;
}

void SetSignalHandlers()
{
    struct { int sig; const char* name; int ignore; } signals[] = {
#if defined(SIGUP)
        { SIGHUP, "SIGHUP", 0 },
#endif
#if defined(SIGINT)
        { SIGINT, "SIGINT", 0 },
#endif
#if defined(SIGPIPE)
        { SIGPIPE, "SIGPIPE", 1 },
#endif
#if defined(SIGALRM)
        { SIGALRM, "SIGALRM", 1 },
#endif
#if defined(SIGTERM)
        { SIGTERM, "SIGTERM", 0 },
#endif
#if defined(SIGUSR1)
        { SIGUSR1, "SIGUSR1", 1 },
#endif
#if defined(SIGUSR2)
        { SIGUSR2, "SIGUSR2", 1 },
#endif
#if defined(SIGPROF)
        { SIGPROF, "SIGPROF", 1 },
#endif
#if defined(SIGVTALRM) 
        { SIGVTALRM, "SIGVTALRM", 1 },
#endif
#ifdef VDE_LINUX
#if defined(SIGPOLL) 
        { SIGPOLL, "SIGPOLL", 1 },
#endif
#ifdef SIGSTKFLT
#if defined(SIGSTKFLT)
        { SIGSTKFLT, "SIGSTKFLT", 1 },
#endif
#endif
#if defined(SIGIO)
        { SIGIO, "SIGIO", 1 },
#endif
#if defined(SIGPWR)
        { SIGPWR, "SIGPWR", 1 },
#endif
#ifdef SIGUNUSED
        { SIGUNUSED, "SIGUNUSED", 1 },
#endif
#endif
        { 0, NULL, 0 }
    };

    uint32_t index;
    for (index = 0; signals[index].sig != 0; index++)
    {
        if (signal(signals[index].sig, signals[index].ignore ? SIG_IGN : sig_handler) < 0)
        {
            strerror_s(errorbuff, sizeof(errorbuff), switch_errno);
            fprintf(stderr, "Setting Handler for %s: %s\n", signals[index].name, errorbuff);
#if defined(LOGGING)
            //printlog(LOG_ERR, "Setting handler for %s: %s", signals[index].name, strerror(switch_errno));
#endif
        }
    }
}

void sig_handler(int sig)
{
    fprintf(stderr, "Caught signal %d, cleaning up and exiting", sig);
    printlog(LOG_ERR, "Caught signal %d, cleaning up and exiting", sig);
    CleanUp();
    signal(sig, SIG_DFL);
    if (sig == SIGTERM)
    {
        _exit(0);
    }
    else
    {   
        TerminateProcess(GetCurrentProcess(), sig);
    }
        
    
}

void StartModules()
{
    StartDataSock();
    StartConsMgmt();
#if defined(HAVE_TUNTAP)

    StartTunTap();
#endif
}

void CleanUp(void)
{
    struct winvde_module* software_modules;
    FileCleanUp();
    for (software_modules = winvde_modules; software_modules != NULL; software_modules = software_modules->next)
    {
        if (software_modules->cleanup != NULL)
        {
            software_modules->cleanup(0, -1, NULL);
        }
    }

}



struct option* optcpy(struct option* tgt, struct option* src, int n, int tag)
{
    int index;
    memcpy(tgt, src, sizeof(struct option) * n);
    for (index = 0; index < n; index++) {
        tgt[index].val = (tgt[index].val & 0xffff) | tag << 16;
    }
    return tgt + n;
}


int parse_globopt(int c, char* optarg)
{
    int outc = 0;
    switch (c) {
    case 'x':
        /* if it is a HUB FST is disabled */
#ifdef FSTP
        fstflag(P_CLRFLAG, FSTP_TAG);
#endif
        portflag(P_SETFLAG, HUB_TAG);
        break;
    case HASH_TABLE_SIZE_ARG:
        sscanf_s(optarg, "%i", &hash_size);
        break;
#ifdef FSTP
    case 'F':
        if (!portflag(P_GETFLAG, HUB_TAG))
            fstflag(P_SETFLAG, FSTP_TAG);
        break;
    case PRIORITY_ARG:
        sscanf(optarg, "%i", &priority);
        priority &= 0xffff;
        break;
#endif
    case MACADDR_ARG:
    {
        int maci[ETH_ALEN], rv;
        if (strchr(optarg, ':') != NULL)
            rv = sscanf_s(optarg, "%x:%x:%x:%x:%x:%x", maci + 0, maci + 1, maci + 2, maci + 3, maci + 4, maci + 5);
        else
            rv = sscanf_s(optarg, "%x.%x.%x.%x.%x.%x", maci + 0, maci + 1, maci + 2, maci + 3, maci + 4, maci + 5);
        if (rv != 6) {
            printlog(LOG_ERR, "Invalid MAC Addr %s", optarg);
            Usage();
        }
        else {
            int index;
            for (index = 0; index < ETH_ALEN; index++)
                switchmac[index] = maci[index];
        }
    }
    break;
    case 'n':
        sscanf_s(optarg, "%i", &numports);
        break;
    case 'v':
        Version();
        break;
    case 'h':
        Usage();
        break;
    default:
        outc = c;
    }
    return outc;
}


void ParseArguments(const int argc, const char** argv)
{
    struct winvde_module * module;
    struct option* long_options;
    char* optstring;
    struct option global_options[] = {
        {"help",0 , 0, 'h'},
        {"hub", 0, 0, 'x'},
#ifdef FSTP
        {"fstp",0 , 0, 'F'},
#endif
        {"version", 0, 0, 'v'},
        {"numports", 1, 0, 'n'},
        {"hashsize", 1, 0, HASH_TABLE_SIZE_ARG},
        {"macaddr", 1, 0, MACADDR_ARG},
#ifdef FSTP
        {"priority", 1, 0, PRIORITY_ARG}
#endif
    };
    static struct option optail = { 0,0,0,0 };
#define N_global_options (sizeof(global_options)/sizeof(struct option))
    prog = (char*)argv[0];
    int totopts = N_global_options + 1;

    for (module = winvde_modules; module != NULL; module = module->next)
    {
        totopts += module->options;
    }
    long_options = malloc(totopts * sizeof(struct option));
    optstring = malloc(2 * totopts * sizeof(char));
    if (long_options == NULL || optstring == NULL)
        exit(2);
    { /* fill-in the long_options fields */
        int index;
        char* os = optstring;
        char last = 0;
        struct option* opp = long_options;
        opp = optcpy(opp, global_options, N_global_options, 0);
        for (module = winvde_modules; module != NULL; module = module->next)
        {
            opp = optcpy(opp, module->module_options, module->options, module->module_tag);
        }
        optcpy(opp, &optail, 1, 0);
        for (index = 0; index < totopts - 1; index++)
        {
            int val = long_options[index].val & 0xffff;
            if (val > ' ' && val <= '~' && val != last)
            {
                *os++ = val;
                if (long_options[index].has_arg) *os++ = ':';
            }
        }
        *os = 0;
    }
    {
        /* Parse args */
        int option_index = 0;
        int c;
        optreset = 1;
        while (1) {
            c = getopt_long(argc, argv, optstring,long_options, &option_index);
            if (c == -1)
                break;
            c = parse_globopt(c, optarg);
            for (module = winvde_modules; module != NULL && c != 0; module = module->next)
            {
                if (module->parse_options!= NULL) {
                    if ((c >> 7) == 0)
                        c = module->parse_options(c, optarg);
                    else if ((c >> 16) == module->module_tag)
                        module->parse_options(c & 0xffff, optarg), c = 0;
                }
            }
        }
    }
    if (optind < argc)
    {
        if (long_options)
        {
            free(long_options);
            long_options = NULL;
        }
        if (optstring)
        {
            free(optstring);
            optstring = NULL;
        }
        Usage();
    }
    if (long_options)
    {
        free(long_options);
        long_options = NULL;
    }
    if (optstring)
    {
        free(optstring);
        optstring = NULL;
    }

}

void Usage()
{
    struct winvde_module * module = NULL;
    printf(
        "Usage: vde_switch [OPTIONS]\n"
        "Runs a VDE switch.\n"
        "(global opts)\n"
        "  -h, --help                 Display this help and exit\n"
        "  -v, --version              Display informations on version and exit\n"
        "  -n  --numports             Number of ports (default %d)\n"
        "  -x, --hub                  Make the switch act as a hub\n"
#ifdef FSTP
        "  -F, --fstp                 Activate the fast spanning tree protocol\n"
#endif
        "      --macaddr MAC          Set the Switch MAC address\n"
#ifdef FSTP
        "      --priority N           Set the priority for FST (MAC extension)\n"
#endif
        "      --hashsize N           Hash table size\n"
        , numports);
    for (module = winvde_modules; module != NULL; module = module->next)
    {
        if (module->usage != NULL)
        {
            module->usage();
        }
    }
    printf(
        "\n"
        "Report bugs to "PACKAGE_BUGREPORT "\n"
    );
    exit(1);

}

void Version()
{
    printf(
        "VDE " PACKAGE_VERSION "\n"
        "Copyright 2003,...,2011 Renzo Davoli\n"
        "some code from uml_switch Copyright (C) 2001, 2002 Jeff Dike and others\n"
        "VDE comes with NO WARRANTY, to the extent permitted by law.\n"
        "You may redistribute copies of VDE under the terms of the\n"
        "GNU General Public License v2.\n"
        "For more information about these matters, see the files\n"
        "named COPYING.\n");
    exit(0);
}

void* mainloop_get_private_data(SOCKET fd)
{
    if (fd >= 0 && fd < fdpermsize)
        return (fdpp[fdperm[fd]]->private_data);
    else
        return NULL;
}

void mainloop_set_private_data(SOCKET fd, void* private_data)
{
    if (fd >= 0 && fd < fdpermsize)
        fdpp[fdperm[fd]]->private_data = private_data;
}

short mainloop_pollmask_get(SOCKET fd)
{
#if DEBUG_MAINLOOP_MASK
    if (fds[fdperm[fd]].fd != fd) printf("PERMUTATION ERROR %d %d\n", fds[fdperm[fd]].fd, fd);
#endif
    return fds[fdperm[fd]].events;
}

void mainloop_pollmask_add(SOCKET fd, short events)
{
#if DEBUG_MAINLOOP_MASK
    if (fds[fdperm[fd]].fd != fd) printf("PERMUTATION ERROR %d %d\n", fds[fdperm[fd]].fd, fd);
#endif
    fds[fdperm[fd]].events |= events;
}

void mainloop_pollmask_del(SOCKET fd, short events)
{
#if DEBUG_MAINLOOP_MASK
    if (fds[fdperm[fd]].fd != fd) printf("PERMUTATION ERROR %d %d\n", fds[fdperm[fd]].fd, fd);
#endif
    fds[fdperm[fd]].events &= ~events;
}

void mainloop_pollmask_set(SOCKET fd, short events)
{
#if DEBUG_MAINLOOP_MASK
    if (fds[fdperm[fd]].fd != fd) printf("PERMUTATION ERROR %d %d\n", fds[fdperm[fd]].fd, fd);
#endif
    fds[fdperm[fd]].events = events;
}


void main_loop()
{
    time_t now;
    int error_count = 0;
    int n=0;
    uint32_t index = 0;
    uint32_t fd_index = 0;
    uint32_t pollable_count = 0;
    uint32_t pollable_pos = 0;
    uint32_t last_descriptor_count = 0;
    int fd_found = 0;
    MSG msg; 
    int prenfds = 0;
    int result = 0;

    fpos_t std_in_pos = 0;
    fpos_t std_last_in_pos = 0;
    DWORD buffer_ready = 0;
    DWORD firstLoop = 1;

    struct pollfd* pollable_fds = NULL;

    

    while (1) {
        if (pollable_fds||firstLoop==1)
        {
            firstLoop = 0;
            switch_errno = 0;
            if (last_descriptor_count != number_of_filedescriptors)
            {
                last_descriptor_count = number_of_filedescriptors;
                free(pollable_fds);
                pollable_count = 0;
                for (index = 0; index < number_of_filedescriptors; index++)
                {
                    
                    if (fds[index].fd > 0 && fds[index].fd != INVALID_SOCKET)
                    {
                        pollable_count++;
                    }
                }
                pollable_fds = (struct pollfd*)malloc(sizeof(struct pollfd) * pollable_count);
                if (pollable_fds)
                {
                    pollable_pos = 0;
                    for (index = 0; index < number_of_filedescriptors; index++)
                    {
                        if (fds[index].fd > 0 && fds[index].fd != INVALID_SOCKET )
                        {
                            if (pollable_pos < pollable_count)
                            {
                                pollable_fds[pollable_pos].fd = fds[index].fd;
                                pollable_fds[pollable_pos].events = fds[index].events;
                                pollable_fds[pollable_pos].revents = 0;
                                pollable_pos++;
                            }
                            else
                            {
                                fprintf(stderr, "Went Past the length of the pollable file descriptor buffer\n");
                                continue;
                            }
                        }
                    }
                }
                else
                {
                    fprintf(stderr, "Failed to get memory for file descriptors\n");
                    break;
                }
            }
            if (!pollable_fds)
            {
                fprintf(stderr, "file descriptors is NULL\n");
                break;
            }
            n = WSAPoll(pollable_fds, pollable_count, 0);
            if (n < 0)
            {
                strerror_s(errorbuff, sizeof(errorbuff), switch_errno);
                printlog(LOG_WARNING, "poll %s %d", errorbuff, WSAGetLastError());
                break;
            }
            else
            {
                now = qtime();
                for (index = 0; n > 0; index++) {
                    fd_index = 0;
                    fd_found = 0;
                    while (1)
                    {
                        if (pollable_fds[index].fd == fdpp[fd_index]->fd)
                        {
                            fd_found = 1;
                            break;
                        }
                        fd_index++;
                        if (fd_index == number_of_filedescriptors)
                        {
                            break;
                        }
                    }
                    if (fd_found == 1)
                    {
                        if (pollable_fds[index].revents != 0 && pollable_fds[index].fd != 0 && pollable_fds[index].fd != INVALID_SOCKET)
                        {
                            prenfds = number_of_filedescriptors;
                            n--;
                            fdpp[fd_index]->timestamp = now;
                            TYPE2MGR(fdpp[fd_index]->index)->handle_io(fdpp[fd_index]->type, fds[fd_index].fd, fds[fd_index].revents, fdpp[fd_index]->private_data);
                            if (number_of_filedescriptors != prenfds) /* the current fd has been deleted */
                            {
                                break; /* PERFORMANCE it is faster returning to poll */
                            }
                        }
                        /* optimization: most used descriptors migrate to the head of the poll array */
#ifdef OPTPOLL
                        else
                        {
                            if (i < number_of_filedescriptors && i > 0 && i != nprio) {
                                int i_1 = i - 1;
                                if (fdpp[i]->timestamp > fdpp[i_1]->timestamp) {
                                    struct pollfd tfds;
                                    struct pollplus* tfdpp;
                                    tfds = fds[i]; fds[i] = fds[i_1]; fds[i_1] = tfds;
                                    tfdpp = fdpp[i]; fdpp[i] = fdpp[i_1]; fdpp[i_1] = tfdpp;
                                    fdperm[fds[i].fd] = i;
                                    fdperm[fds[i_1].fd] = i_1;
                                }
                            }
                        }
#endif
                    }
                }
            }
        }
        if (_kbhit())
        {
            std_input_buffer[std_input_pos] = _getche();
            if (std_input_buffer[std_input_pos] == '\n' || std_input_buffer[std_input_pos] == '\r')
            {
                buffer_ready = 1;
            }
            else if (std_input_pos >= BUFFER_SIZE - 1)
            {
                buffer_ready = 1;
            }
            else
            {
                std_input_pos++;
            }
        }
        if (buffer_ready == 1)
        {
            if (std_input_pos > 0)
            {
                if (memcmp(std_input_buffer, "quit", min(std_input_pos, strlen("quit"))) == 0)
                {
                    break;
                }
            }
            buffer_ready = 0;
            std_input_pos = 0;
        }
        // required for qtimer to work
        if (!GetMessage(&msg, NULL, 0, 0))
        {
            break;
        }
        DispatchMessage(&msg);
        TranslateMessage(&msg);
    }
}

