#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
#include <winvdeplugmodule.h>

__declspec(dllexport) WINVDECONN* winvde_vde_open(const char* given_winvde_url, char* descr, int interface_version, struct winvde_open_args* winvde_open_args);
__declspec(dllexport) size_t winvde_vde_recv(WINVDECONN* conn, char* buff, size_t len, int flags);
__declspec(dllexport) size_t winvde_vde_send(WINVDECONN* conn, const char* buff, size_t len, int flags);
__declspec(dllexport) int winvde_vde_datafd(WINVDECONN* conn);
__declspec(dllexport) int winvde_vde_ctlfd(WINVDECONN* conn);
__declspec(dllexport) int winvde_vde_close(WINVDECONN* conn);
