#pragma once 
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <WinSock2.h>
#include <stdint.h>

#define EPOLL_CTL_ADD 1
#define EPOLL_CTL_DEL 2
#define EPOLL_CTL_MOD 3

#define EPOLLIN			0x80000000
#define EPOLLOUT		0x40000000
#define EPOLLET			0x20000000
#define EPOLLONESHOT	0x10000000
#define EPOLLHUP		0x08000000
#define EPOLLRDHUP		0x04000000
#define EPOLLPRI		0x02000000
#define EPOLLERR		0x01000000

#define EPOLLANY		0xFF000000
#include <pshpack1.h>
union epoll_uint128
{
	char c_data[16];
	uint16_t ui16[8];
	uint32_t ui32[4];
	uint64_t ui64[2];
};
union epoll_data {
	void* ptr;
	SOCKET file_descriptor;
	uint32_t u32;
	uint64_t u64;
	union epoll_uint128 u128;
};

struct epoll_event {
	uint32_t events;
	union epoll_data data;
};

struct epoll_file_descriptors
{
	int file_descriptor;
	uint32_t number_of_events;
	struct epoll_event* event;
	struct epoll_file_descriptors* next;
};
#include <poppack.h>


int epoll_create(int size);

int epoll_wait(int ipfd, struct epoll_event** events, int max_events, int timout);

int epoll_ctl(int ipfd, int op, SOCKET fd, struct epoll_event* event);

int get_next_descriptor();

int epoll_release(int ipfd);

struct epoll_file_descriptors* get_epoll_descriptor(int file_desctiptor);