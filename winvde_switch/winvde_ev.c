#include <WinSock2.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <io.h>
#include <stdlib.h>

#include "winvde_ev.h"
#include "winvde_mgmt.h"


size_t readv(SOCKET fd, struct iovec* iov, int iovcnt)
{
	size_t bytesRead = 0;
	size_t totalBytesRead = 0;
	int index = 0;
	if (fd == INVALID_SOCKET||iov ==NULL || iovcnt<=0)
	{
		switch_errno = EINVAL;
		return -1;
	}
	for (index = 0; index < iovcnt; index++)
	{
		bytesRead = recv(fd, (char*) & iov[index].iov_len, (int)sizeof(size_t), 0);
		if (bytesRead != sizeof(size_t))
		{
			break;
		}
		totalBytesRead += bytesRead;
		if (iov[index].iov_len > 0)
		{
			iov[index].iov_base = (char*)malloc(iov[index].iov_len);
			if (!iov[index].iov_base)
			{
				switch_errno = ENOMEM;
				break;
			}
			bytesRead = recv(fd, iov[index].iov_base, (int)iov[index].iov_len,0);
			if (bytesRead != iov[index].iov_len)
			{
				switch_errno = EFAULT;
				break;
			}
		}
		else
		{
			iov[index].iov_base = NULL;
		}
		totalBytesRead += bytesRead;
	}
	return totalBytesRead;
}

size_t writev(SOCKET fd, struct iovec* iov, int iovcnt)
{
	size_t bytesWritten = 0;
	size_t totalBytesWritten = 0;
	int index = 0;
	if (fd == -1 || iov == NULL || iovcnt <= 0)
	{
		switch_errno = EINVAL;
		return -1;
	}
	for (index = 0; index < iovcnt; index++)
	{
		bytesWritten = send(fd, (char*) & iov[index].iov_len, sizeof(size_t), 0);
		if (bytesWritten != sizeof(size_t))
		{
			switch_errno = EFAULT;
			break;
		}
		totalBytesWritten += bytesWritten;
		if (iov[index].iov_len > 0)
		{
			bytesWritten = send(fd, iov[index].iov_base, (uint32_t)iov[index].iov_len,0);
			if (bytesWritten != iov[index].iov_len)
			{
				switch_errno = EFAULT;
				break;
			}
			totalBytesWritten += bytesWritten;
		}
	}
	return totalBytesWritten;
}