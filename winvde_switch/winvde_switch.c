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

// custom includes
#include "winvde_mgmt.h"
#include "winvde_datasock.h"
#include "winvde_module.h"
#include "winvde_switch.h"
#include "winvde_hash.h"
#include "winvde_port.h"
#include "winvde_loglevel.h"
#include "options.h"
#include "getopt.h"
#include "version.h"

// add libraries
#pragma comment (lib,"ws2_32.lib")

// constants
/* FD MGMT */
struct pollplus {
    unsigned char type;
    void* private_data;
    time_t timestamp;
};

static struct pollfd* fds = NULL;
static struct pollplus** fdpp = NULL;

#define PRIOFLAG 0x80
#define TYPEMASK 0x7f
#define ISPRIO(X) ((X) & PRIOFLAG)

#define TYPE2MGR(X) (winvde_modules[((X) & TYPEMASK)])
// global variables
unsigned char switchmac[ETH_ALEN];

char errorbuff[1024];
int hash_size = INIT_HASH_SIZE;
static int numports = INIT_NUMPORTS;
char* prog;
int nfds = 0;

static uint64_t xrand48 = 0;
static uint64_t xx = 0;

uint32_t number_of_filedescriptors = 0;

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

#if !defined(TEST_QTIMER) && !defined(TEST_BITARRAY)
int main(const int argc, const char ** argv)
{
    WSADATA wsadata;
    if (argc <= 0)
    {
        fprintf(stderr, "Invalid EXE\n");
        exit(0xFFFFFFFF);
    }

    prog = argv[0];

    memset(&wsadata,0,sizeof(WSADATA));

    if (WSAStartup(MAKEWORD(2, 2), &wsadata) == SOCKET_ERROR)
    {
        fprintf(stderr, "Failed to start up sockets\n");
        exit(0xFFFFFFF);
    }

    SetMacAddress();

    fprintf(stdout, "Ethernet Switch Mac: %02x:%02x:%02x:%02x:%02x:%02x", switchmac[0], switchmac[1], switchmac[2], switchmac[3], switchmac[4], switchmac[5] );

    SetSignalHandlers();

    fprintf(stdout, "Signal handlers set\n");

    StartModules();

    fprintf(stdout, "Modules Started\n");

    ParseArguments(argc,argv);

    fprintf(stdout, "Arguments Parsed\n");

    atexit(CleanUp);

    HashInit(hash_size);

#ifdef FSTP
    fst_init(numports);
#endif

    port_init(numports);


    init_mods();

    loadrcfile();
    
    qtimer_init();
    
    main_loop();

    WSACleanup();
  
}

#endif

void SetMacAddress()
{
    struct timeval v;
    unsigned long val;
    int index;

    gettimeofday(&v);
    srand48(v.tv_sec ^ v.tv_usec ^ _getpid());
    for (index = 0, val = lrand48(); index < 4; index++, val >>= 8)
    {
        switchmac[index + 2] = val;
    }
    switchmac[0] = 0;
    switchmac[1] = 0xff;

}

int gettimeofday(struct timeval* tv)
{
    FILETIME lpFileTime;
    ULARGE_INTEGER timeValue64;
    ULONGLONG total;
    const ULONGLONG epoch_offset = 11644473600000000ULL;
    if (!tv)
    {
        errno = EINVAL;
        return -1;
    }
#if definde WIN32_WINNT >= _WIN32_WINNT_WIN8
    GetSystemTimePreciseAsFileTime(&lpFileTime);
#else
    GetSystemTimeAsFileTime(&lpFileTime);
#endif

    timeValue64.HighPart = lpFileTime.dwHighDateTime;
    timeValue64.LowPart = lpFileTime.dwLowDateTime;

    total = timeValue64.QuadPart / 10 - epoch_offset;
    tv->tv_sec = (time_t)(total / 1000000ULL);
    tv->tv_usec = (long)(total / 1000000ULL);
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
            strerror_s(errorbuff, sizeof(errorbuff), errno);
            fprintf(stderr, "Setting Handler for %s: %s\n", signals[index].name, errorbuff);
#if defined(LOGGING)
            //printlog(LOG_ERR, "Setting handler for %s: %s", signals[index].name, strerror(errno));
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
    StartConsMgmt();
    StartDataSock();
    StartTunTap();
}

void CleanUp()
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

void FileCleanUp()
{
    uint32_t index = 0;
    struct winvde_module* module;
    for (index = 0; index < number_of_filedescriptors; index++)
    {
        module = &TYPE2MGR(fdpp[index]->type);
        if (module != NULL)
        {
            module->cleanup(fdpp[index]->type, fds[index].fd, fdpp[index]->private_data);
        }
    }
}

struct option* optcpy(struct option* tgt, struct option* src, int n, int tag)
{
    int i;
    memcpy(tgt, src, sizeof(struct option) * n);
    for (i = 0; i < n; i++) {
        tgt[i].val = (tgt[i].val & 0xffff) | tag << 16;
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
            int i;
            for (i = 0; i < ETH_ALEN; i++)
                switchmac[i] = maci[i];
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
    prog = argv[0];
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
        int i;
        char* os = optstring;
        char last = 0;
        struct option* opp = long_options;
        opp = optcpy(opp, global_options, N_global_options, 0);
        for (module = winvde_modules; module != NULL; module = module->next)
        {
            opp = optcpy(opp, module->module_options, module->options, module->module_tag);
        }
        optcpy(opp, &optail, 1, 0);
        for (i = 0; i < totopts - 1; i++)
        {
            int val = long_options[i].val & 0xffff;
            if (val > ' ' && val <= '~' && val != last)
            {
                *os++ = val;
                if (long_options[i].has_arg) *os++ = ':';
            }
        }
        *os = 0;
    }
    {
        /* Parse args */
        int option_index = 0;
        int c;
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
        Usage();
    }
    free(long_options);
    free(optstring);

}


void StartTunTap(void)
{

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

void main_loop()
{
    time_t now;
    int n, i;
    while (1) {
        n = select(nfds, fds, NULL,NULL,NULL);
        now = qtime();
        if (n < 0) {
            if (errno != EINTR)
            {
                strerror_s(errorbuff, sizeof(errorbuff), errno);
                printlog(LOG_WARNING, "poll %s", errorbuff);
            }
        }
        else {
            for (i = 0; /*i < nfds &&*/ n > 0; i++) {
                if (fds[i].revents != 0) {
                    int prenfds = nfds;
                    n--;
                    fdpp[i]->timestamp = now;
                    TYPE2MGR(fdpp[i]->type).handle_io(fdpp[i]->type, fds[i].fd, fds[i].revents, fdpp[i]->private_data);
                    if (nfds != prenfds) /* the current fd has been deleted */
                        break; /* PERFORMANCE it is faster returning to poll */
                }
                /* optimization: most used descriptors migrate to the head of the poll array */
#ifdef OPTPOLL
                else
                {
                    if (i < nfds && i > 0 && i != nprio) {
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

