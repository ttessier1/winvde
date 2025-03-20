#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winmeta.h>
#include <WinSock2.h>
#include <afunix.h>
#include <stdlib.h>
#include <stdio.h>
#include <direct.h>
#include "winvde_sockutils.h"
#include "winvde_loglevel.h"
#include "winvde_switch.h"
#include "winvde_output.h"

/* check to see if given unix socket is still in use; if it isn't, remove the
 *  * socket from the file system */
int still_used(struct sockaddr_un* sun)
{
	SOCKET test_fd = INVALID_SOCKET;
	int ret = 1;

	if ((test_fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		strerror_s(errorbuff, sizeof(errorbuff), errno);
		printlog(LOG_ERR, "socket %s", errorbuff);
		return(1);
	}
	if (connect(test_fd, (struct sockaddr*)sun, sizeof(*sun)) < 0) {
		if (errno == ECONNREFUSED) {
			if (_unlink(sun->sun_path) < 0) {
				strerror_s(errorbuff,sizeof(errorbuff),errno);
				printlog(LOG_ERR, "Failed to removed unused socket '%s': %s",sun->sun_path, errorbuff);
			}
			ret = 0;
		}
		else
		{
			strerror_s(errorbuff, sizeof(errorbuff), errno);
			printlog(LOG_ERR, "connect %s", errorbuff);
		}
	}
	closesocket(test_fd);
	return(ret);
}


int strlength(const char*string)
{
	int length = 0;
	if (!string)
	{
		return 0;
	}
	while (*string != '\0')
	{
		string++;
		length++;
	}
	return length;
}