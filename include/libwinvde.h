#pragma once

#include <stdint.h>

#define LIBWINVDE_PLUG_VERSION 1

#define WINVDE_MAXMTU 9216
#define WINVDE_ETHBUFSIZE (WINVDE_MAXMTU + 14 + 4) // + Ethernet header + 802.1Q header


struct winvdeconn;

typedef struct winvdeconn WINVDECONN;

struct winvde_open_args {
	int port;
	char * group;
	int mode;
};

#define winvde_open(vnl,desc,winvde_open_args)\
	winvde_open_real((vnl),(desc),LIBWINVDE_PLUG_VERSION,(open_args))

WINVDECONN* winvde_open_real(const char* vnl, const char* desc, int interface_version, struct winvde_open_args * open_args);

size_t winvde_recv(WINVDECONN* conn, const char * buf, size_t len, uint32_t flags);

size_t winvde_send(WINVDECONN* conn, const char * buf, size_t len, uint32_t flags);

int winvde_datafiledescriptor(WINVDECONN* conn);

int winvde_ctlfiledescriptor(WINVDECONN* conn);

int winvde_close(WINVDECONN*conn);

struct winvdestream;

typedef struct winvdestream WINVDESTREAM;

#define PACKET_LENGTH_ERROR 1

WINVDESTREAM* winvdestream_open(void* opaque, int filedescriptorout, size_t(*frecv)(void* opaque, void* buff, size_t count), void (*ferr)(void* opaque, int type, char* format, ...));

size_t winvdestream_send(WINVDESTREAM * winvdestream,const unsigned char * buff, size_t length);

size_t winvdestream_recv(WINVDESTREAM * winvdestream, unsigned char * buff, size_t length);

void winvdestream_close(WINVDESTREAM * winvdestream);
