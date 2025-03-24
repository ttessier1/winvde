
#pragma once

struct iovec {
	char* iov_base; 
	size_t  iov_len;
};

size_t readv(SOCKET fd, struct iovec* iov, int iovcnt);
size_t writev(SOCKET fd, struct iovec* iov, int iovcnt);
