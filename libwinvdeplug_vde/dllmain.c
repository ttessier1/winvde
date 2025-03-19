// dllmain.cpp : Defines the entry point for the DLL application.

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <stdio.h>
#include <process.h>
#include <afunix.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <libwinvde.h>
#include <winvdeplugmodule.h>
#include "framework.h"

#pragma comment(lib,"winvde.lib")
#pragma comment(lib,"ws2_32.lib")

struct winvdeplug_module winvdeplug_opts = {
    .winvde_open_real = winvde_vde_open,
    .winvde_recv = winvde_vde_recv,
    .winvde_send = winvde_vde_send,
    .winvde_datafiledescriptor = winvde_vde_datafd,
    .winvde_ctlfiledescriptor = winvde_vde_ctlfd,
    .winvde_close = winvde_vde_close,
};


struct winvde_vde_conn
{
    void* handle;
    struct winvdeplug_module module;
    int filedescriptorctl;
    int filedescriptordata;
};

static const char* fallback_winvde_url[] =
{
    "\\windows\\tmp\\winvde",
    "%USERPROFILE%\\.winvde\\winvde",
    "%USERPROFILE%\\appdata\\local\\temp\\winvde",
    NULL,
};

static const char* fallback_dirname[] = {
    "\\windows\\temp",
    "%USERPROFILE%\temp",
    NULL,
};

#define SWITCH_MAGIC 0xbeefface

enum request_type { REQ_NEW_CONTROL, REQ_NEW_PORT0};

#include <pshpack1.h>

struct request_v3
{
    uint32_t magic;
    uint32_t version;
    enum request_type type;
    struct sockaddr_un sock;
    char description[MAXDESCR];
};

#include <poppack.h>

int try2connect(int filedescriptorctl, const char* url, const char* portgroup, int port, struct sockaddr_un* ctlsock)
{
    int res = -1;
    if (ctlsock != NULL)
    {
        memset(ctlsock, 0, sizeof(struct sockaddr_un));
        ctlsock->sun_family = AF_UNIX;
        
        if (portgroup)
        {
            snprintf(ctlsock->sun_path, sizeof(ctlsock->sun_path), "%s\\%s", url, portgroup);
            res = connect(filedescriptorctl, (const struct sockaddr*)ctlsock, sizeof(struct sockaddr_un));
        }
        if (res < 0 || port != 0)
        {
            snprintf(ctlsock->sun_path, sizeof(ctlsock->sun_path), "%s\\ctl", url);
            res = connect(filedescriptorctl, (const struct sockaddr*)ctlsock, sizeof(struct sockaddr_un));
        }
        if (res == 0) { res = 1; }

        
    }
    return res;
}


WINVDECONN* winvde_vde_open(const char* given_winvde_url, const char* descr, int interface_version, struct winvde_open_args * winvde_open_args)
{
    DebugBreak();
    WSADATA wsadata;
    int result = 0;
    struct request_v3 request;
    int port = 0;
    char* portgroup = NULL;
    const uint8_t str_lenofint = snprintf(NULL, 0, "%d ", 0xFFFFFFFF);
    char * numeric_portgroup;
    char* group = NULL;
    char* modestr = NULL;
    uint32_t mode = 0700;
    //char real_winvde_url[MAX_PATH];
    int filedescriptorctl = -1;
    int filedescriptordata = -1;
    struct sockaddr_un sockun;
    struct sockaddr_un datasock;
    const char* winvde_url = NULL;
    int res = 0;
    int pid = _getpid();
    int sockno = 0;
    char temppath[MAX_PATH];
    struct winvde_vde_conn* newconn = NULL;
    struct winvde_parameters parameters[] = {
        {(char*)"",&portgroup}, 
        {(char*)"port", &portgroup},
        {(char*)"portgroup",&portgroup},
        {(char*)"group",&group},
        {(char*)"mode",&modestr},
        {NULL,NULL},
    };

    numeric_portgroup = (char *)malloc(str_lenofint+1);
    if (!numeric_portgroup)
    {
        return NULL;
    }

    if (winvde_open_args != NULL)
    {
        if (interface_version == 1)
        {
            port = winvde_open_args->port;
            group = winvde_open_args->group;
            mode = winvde_open_args->mode;
        }
        else
        {
            errno = EINVAL;
            goto abort;
        }
    }

    if (winvde_parsepathparameters((char*)given_winvde_url, parameters) < 0) {
        errno = EINVAL;
        goto abort;
    }
    if (modestr) {
        mode = strtol(modestr,NULL,8);
    }

    // * this is not correct
    //if (*given_vde_url )
    //{
    //    retval = GetFullPathName(*given_vde_url,MAX_PATH,winvde_url,lppPart);
    //}
    //winvde_url = real_vde_url;

    if ((portgroup == NULL || *portgroup == 0) && port != 0)
    {
        snprintf(numeric_portgroup, str_lenofint, "%d ", port < 0 ? 0 : port);
        portgroup = numeric_portgroup;
    }
    else if (portgroup != NULL && isdigit(*portgroup))
    {
        char* endptr;
        int portgroup2port = strtol(portgroup,&endptr,10);
        if (*endptr == 0)
        {
            port = portgroup2port;
            if (port == 0)
            {
                port = -1;
            }
        }
    }

    memset(&wsadata, 0, sizeof(WSADATA));


    result = WSAStartup(MAKEWORD(2, 2), &wsadata);
    if (result != 0)
    {
        goto abort;
    }
    if ((filedescriptorctl = (int)socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        goto abort;
    }
    if ((filedescriptordata = (int)socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        goto abort;
    }

    if (*given_winvde_url)
    {
        res = try2connect(filedescriptorctl, winvde_url, portgroup, port, &sockun);
    }
    else
    {
        int index = 0;
        for (index = 0, res = -1; fallback_winvde_url[index] && (res < 0); index++)
        {
            if (ExpandEnvironmentStringsA(fallback_winvde_url[index], temppath, MAX_PATH) != 0)
            {
                winvde_url = temppath;
                res = try2connect(filedescriptorctl, winvde_url, portgroup, port, &sockun);
                fprintf(stderr, "Failed to connect: %d\n", WSAGetLastError());
            }

        }
    }

    if (res < 0)
    {
        goto abort;
    }

    request.magic = SWITCH_MAGIC;
    request.version = 3;
    request.type = REQ_NEW_CONTROL;
    if (res > 0 && port != 0)
    {
        if (port < 0)
        {
            request.type = REQ_NEW_PORT0;
        }
        else
        {
            request.type = (request.type + (port<< 8));
        }
    }
    strncpy_s(request.description, sizeof(request.description), descr,MAXDESCR);
    datasock.sun_family = AF_UNIX;
    memset(datasock.sun_path, 0, sizeof(datasock.sun_path));
    do
    {
        snprintf(datasock.sun_path, sizeof(datasock.sun_path), "%s\\.%05d-%05d", winvde_url, pid, sockno++);
    } while (res < 0 && errno == EADDRINUSE);

    if(res<0)
    {
        int index = 0;
        for (index = 0, res = -1, sockno = 0; fallback_dirname[index] && (res != 0); index++)
        {
            memset(datasock.sun_path, 0, sizeof(datasock.sun_path));
            do
            {
                snprintf(datasock.sun_path, sizeof(datasock.sun_path), "%s\\vde.%05d-%05d", fallback_dirname[index], pid, sockno++);
                res = bind(filedescriptordata, (struct sockaddr*)&datasock, sizeof(datasock));

            } while (res < 0 && errno == EADDRINUSE);

        }
    
    }
    if (res < 0)
    {
        goto abort;
    }
    /*if (group)
    {
        struct group * gs;
        uint32_t gid = 0 ;
        // windows does not have group ids so...
        if ((gs = getgrname(group)) == NULL)
        {
            gid = atoi(group);
        }
        else
        {
            gid = gs->gr_gid;
        }
        if(chown(datasock.sun+path,-1,gid))<0)
        {
            goto abort_deletesock;
        }
        
    }
    else
    {
        struct _stat64 ctlstat;
        if ((mode & 077) == 0)
        {
            if (stat(sockun.sun_path, &ctlstat)==0)
            {
                if (ctlstat.st_uid != 0 && ctlstat.st_uid!= geteuid())
                {
                    if (chown(datasock.sun_path, -1, ctlstat.st_gid) == 0)
                    {
                        mode |= 070;
                    }
                    else
                    {
                        mode |= 077;
                    }
                }
            }
        }

    }
    chmod(datasock.sun_path,mode);
    */
    request.sock = datasock;

    if (send(filedescriptorctl,(const char *) & request,(int) (sizeof(request) - MAXDESCR + strlen(request.description)), 0) < 0)
    {
        goto abort_deletesock;
    }

    if (recv(filedescriptorctl, (char*) & datasock, sizeof(struct sockaddr_un), 0) <= 0)
    {
        goto abort_deletesock;
    }

    if (connect(filedescriptordata, (struct sockaddr*) & datasock, (int)sizeof(struct sockaddr_un)) < 0)
    {
        goto abort_deletesock;
    }

    // chmod(datasock.sun_path,mode);

    if ((newconn = (struct winvde_vde_conn *)malloc(sizeof(struct winvde_vde_conn))) == NULL)
    {
        errno = ENOMEM;
        goto abort_deletesock;
    }

    newconn->filedescriptorctl = filedescriptorctl;
    newconn->filedescriptordata = filedescriptordata;
    _unlink(request.sock.sun_path);
    if (numeric_portgroup)
    {
        free(numeric_portgroup);
        numeric_portgroup = NULL;
    }
    return (WINVDECONN*)newconn;
abort_deletesock:
    _unlink(request.sock.sun_path);
abort:
    if (filedescriptorctl >= 0) closesocket(filedescriptorctl);
    if (filedescriptordata >= 0) closesocket(filedescriptordata);
    if (numeric_portgroup)
    {
        free(numeric_portgroup);
        numeric_portgroup = NULL;
    }
    WSACleanup();
    return NULL;
}

size_t winvde_vde_recv(WINVDECONN* conn, const char * buff, size_t len, int flags)
{
    DebugBreak();
    if (!conn || !buff)
    {
        errno=E_INVALIDARG;
        return -1;
    }
    struct winvde_vde_conn* winvde_conn = (struct winvde_vde_conn*)conn;
    return recv(winvde_conn->filedescriptordata, buff, (int)len, 0);
}

size_t winvde_vde_send(WINVDECONN* conn, const char* buff, size_t len, int flags)
{
    DebugBreak();
    if (!conn || !buff)
    {
        errno = E_INVALIDARG;
        return -1;
    }
    struct winvde_vde_conn* winvde_conn = (struct winvde_vde_conn*)conn;
    return send(winvde_conn->filedescriptordata, buff, (int)len, 0);
}

int winvde_vde_datafd(WINVDECONN* conn)
{
    if (!conn)
    {
        errno = E_INVALIDARG;
        return -1;
    }
    struct winvde_vde_conn* winvde_conn = (struct winvde_vde_conn*)conn;
    return winvde_conn->filedescriptordata;
}

int winvde_vde_ctlfd(WINVDECONN* conn)
{
    if (!conn)
    {
        errno=E_INVALIDARG;
        return -1;
    }
    struct winvde_vde_conn* winvde_conn = (struct winvde_vde_conn*)conn;
    return winvde_conn->filedescriptorctl;
}

int winvde_vde_close(WINVDECONN* conn)
{
    if (!conn)
    {
        errno = E_INVALIDARG;
        return -1;
    }
    struct winvde_vde_conn* winvde_conn = (struct winvde_vde_conn*)conn;
    closesocket(winvde_conn->filedescriptordata);
    closesocket(winvde_conn->filedescriptorctl);
    
    FreeLibrary(winvde_conn->handle);
    free(winvde_conn);
    return 0;
}



BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}



