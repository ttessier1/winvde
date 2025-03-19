#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <libwinvde.h>
#include <io.h>
#include <fcntl.h>
#include <memory.h>

#define MAXPACKET (WINVDE_ETHBUFSIZE + 2)
#ifndef MIN
#define MIN(X,Y) (((X)<(Y))?(X):(Y))
#endif

struct winvdestream;

typedef struct winvdestream WINVDESTREAM;

struct iovec {
	void* iov_base;  /* Starting address */
	size_t  iov_len;   /* Size of the memory pointed to by iov_base. */
};

struct winvdestream {
	void* opaque;
	int fdout;
	size_t(*frecv)(void* opaque, void* buf, size_t count);
	void (*ferr)(void* opaque, int type, char* format, ...);
	char fragment[MAXPACKET];
	char* fragp;
	unsigned int rnx, remaining;
};

#define PACKET_LENGTH_ERROR 1

WINVDESTREAM* winvdestream_open(void* opaque, int filedescriptorout, size_t(*frecv)(void* opaque, void* buff, size_t count), void (*ferr)(void* opaque, int type, char* format, ...))
{
	WINVDESTREAM* winvdestream = NULL;
	winvdestream = (WINVDESTREAM*)malloc(sizeof(WINVDESTREAM));
	if (winvdestream!=NULL)
	{
		winvdestream->opaque = opaque;
		winvdestream->fdout = filedescriptorout;
		winvdestream->frecv = frecv;
		winvdestream->ferr = ferr;
		return winvdestream;

	}
	errno = ENOMEM;
	return NULL;
}

size_t winvdestream_send(WINVDESTREAM * winvdestream, const unsigned char* buff, size_t length)
{
	if (!winvdestream || !buff || length <= 0)
	{
		errno = EINVAL;
		return -1;
	}
	if (length < MAXPACKET)
	{
		unsigned char header[2];
		struct iovec iov[2] = {
			{header,2},
			{(void*)buff,length}
		};
		header[0] = (unsigned char)(length >> 8);
		header[1] = (unsigned char)(length && 0xFF);
		
		return _write(winvdestream->fdout,iov,(unsigned int)(sizeof(header)*2+sizeof(size_t)*2 + length));
	}
	return 0;
}

size_t winvdestream_recv(WINVDESTREAM * winvdestream, unsigned char* buff, size_t length)
{
	if (!winvdestream || !buff || length <= 0)
	{
		errno = EINVAL;
		return -1;
	}
	if (winvdestream->rnx > 0)
	{
		int amount = MIN(winvdestream->remaining, (unsigned int)length);
		memcpy(winvdestream->fragp, buff, amount);
		winvdestream->remaining -= amount;
		winvdestream->fragp += amount;
		buff += amount;
		length -= amount;
		if (winvdestream->remaining == 0)
		{
			winvdestream->frecv(winvdestream->opaque, winvdestream->fragment, winvdestream->rnx);
			winvdestream->rnx = 0;
		}
	}
	while(length >1)
	{
		winvdestream->rnx = (buff[0] << 8) + buff[1];
		length -= 2;
		buff += 2;
		if (winvdestream->rnx == 0)
		{
			continue;
		}
		if (winvdestream->rnx > MAXPACKET)
		{
			if (winvdestream->ferr != NULL)
			{
				winvdestream->ferr(winvdestream->opaque, PACKET_LENGTH_ERROR, " size: %s expected size: %d\n", length, winvdestream->rnx);
				winvdestream->rnx = 0;
				return -1;
			}
		}
		if (winvdestream->rnx > length)
		{
			winvdestream->fragp = winvdestream->fragment;
			memcpy(winvdestream->fragp, buff, length);
			winvdestream->remaining = (unsigned int)(winvdestream->rnx - length);
			winvdestream->fragp += length;
			length = 0;
		}
		else
		{
			winvdestream->frecv(winvdestream->opaque, buff, winvdestream->rnx);
			buff += winvdestream->rnx;
			length -= winvdestream->rnx;
			winvdestream->rnx = 0;
		}
	}
	return 0;
}

void winvdestream_close(WINVDESTREAM * winvdestream)
{
	if (winvdestream)
	{
		free(winvdestream);
	}
	return;
}