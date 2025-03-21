#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <io.h>
#include <stdlib.h>
#include "winvde_ev.h"


size_t readv(int fd, struct iovec* iov, int iovcnt)
{
	size_t bytesRead = 0;
	size_t totalBytesRead = 0;
	int index = 0;
	if (fd == -1||iov ==NULL || iovcnt<=0)
	{
		errno = EINVAL;
		return -1;
	}
	for (index = 0; index < iovcnt; index++)
	{
		bytesRead = _read(fd, &iov[index].iov_len, sizeof(size_t));
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
				errno = ENOMEM;
				break;
			}
			bytesRead = _read(fd, iov[index].iov_base, iov[index].iov_len);
			if (bytesRead != iov[index].iov_len)
			{
				errno = EFAULT;
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
size_t writev(int fd, const struct iovec* iov, int iovcnt)
{
	size_t bytesWritten = 0;
	size_t totalBytesWritten = 0;
	int index = 0;
	if (fd == -1 || iov == NULL || iovcnt <= 0)
	{
		errno = EINVAL;
		return -1;
	}
	for (index = 0; index < iovcnt; index++)
	{
		bytesWritten = _write(fd, iov[index].iov_len, sizeof(size_t));
		if (bytesWritten != sizeof(size_t))
		{
			errno = EFAULT;
			break;
		}
		totalBytesWritten += bytesWritten;
		if (iov[index].iov_len > 0)
		{
			bytesWritten = _write(fd, iov[index].iov_base, iov[index].iov_len);
			if (bytesWritten != iov[index].iov_len)
			{
				errno = EFAULT;
				break;
			}
			totalBytesWritten += bytesWritten;
		}
	}
	return totalBytesWritten;
}