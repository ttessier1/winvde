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
#include <libwinvdeplug_netnode.h>

#include "framework.h"


#pragma comment(lib,"winvde.lib")
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"winvde_netnode.lib")

#if !defined TRUE
#define TRUE 1
#endif

#if !defined FALSE
#define FALSE 0
#endif


const struct winvdeplug_module * winvdeplug_ops = &winvdeplug_switch_ops;

WINVDECONN* winvde_vde_open(char* vde_url, char* descr, int interface_version, struct vde_open_args* open_args)
{
    return winvdeplug_ops->winvde_open_real(vde_url, descr, interface_version, open_args);
}

size_t winvde_vde_recv(WINVDECONN* conn, void* buf, size_t len, int flags)
{
    return winvdeplug_ops->winvde_recv(conn, buf, len, flags);
}

/* There is an avent coming from the host */

size_t winvde_vde_send(WINVDECONN* conn, const void* buf, size_t len, int flags)
{
    return winvdeplug_ops->winvde_send(conn, buf, len, flags);
}

int winvde_vde_datafd(WINVDECONN* conn)
{
    return winvdeplug_ops->winvde_datafiledescriptor(conn);
}

int winvde_vde_ctlfd(WINVDECONN* conn)
{
    return winvdeplug_ops->winvde_ctlfiledescriptor(conn);
}

int winvde_vde_close(WINVDECONN* conn)
{
    return winvdeplug_ops->winvde_close(conn);
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

