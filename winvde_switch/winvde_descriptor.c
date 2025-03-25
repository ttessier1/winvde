#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>
#include <winmeta.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "winvde_descriptor.h"
#include "winvde_loglevel.h"
#include "winvde_output.h"
#include "winvde_switch.h"

int maxfds = 0;
int nprio = 0;

struct pollfd* fds = NULL;

struct pollplus** fdpp = NULL;

#define FDPERMSIZE_LOGSTEP 4

short* fdperm;
uint64_t fdpermsize = 0;

uint32_t number_of_filedescriptors = 0;

void add_fd(SOCKET fd, unsigned char type, unsigned char module_index, void* private_data)
{
	struct pollfd* p;
	int index;
	/* enlarge fds and fdpp array if needed */
	if (number_of_filedescriptors == maxfds) {
		maxfds = maxfds ? maxfds + MAXFDS_STEP : MAXFDS_INITIAL;
		if ((fds = (struct pollfd*)realloc(fds, maxfds * sizeof(struct pollfd))) == NULL) {
			strerror_s(errorbuff, sizeof(errorbuff), errno);
			printlog(LOG_ERR, "realloc fds %s", errorbuff);
			exit(1);
		}
		if ((fdpp = (struct pollplus**)realloc(fdpp, maxfds * sizeof(struct pollplus*))) == NULL) {
			strerror_s(errorbuff, sizeof(errorbuff), errno);
			printlog(LOG_ERR, "realloc pollplus %s", errorbuff);
			exit(1);
		}
	}
	if (fd >= fdpermsize)
	{
		fdpermsize = ((fd >> FDPERMSIZE_LOGSTEP) + 1) << FDPERMSIZE_LOGSTEP;
		if ((fdperm = (short*)realloc(fdperm, fdpermsize * sizeof(short))) == NULL)
		{
			strerror_s(errorbuff,sizeof(errorbuff),errno);
			printlog(LOG_ERR, "realloc fdperm %s", errorbuff);
			exit(1);
		}
	}
	if (ISPRIO(type))
	{
		fds[number_of_filedescriptors] = fds[nprio];
		fdpp[number_of_filedescriptors] = fdpp[nprio];
		index = nprio;
		nprio++;
	}
	else
	{
		index = number_of_filedescriptors;
	}
	if ((fdpp[index] = malloc(sizeof(struct pollplus))) == NULL) {
		strerror_s(errorbuff, sizeof(errorbuff), errno);
		printlog(LOG_ERR, "realloc pollplus elem %s", errorbuff);
		exit(1);
	}
	fdperm[fd] = index;
	p = &fds[index];
	p->fd = fd;
	p->events = POLLIN | POLLHUP;
	fdpp[index]->index = module_index;
	fdpp[index]->type = type;
	fdpp[index]->private_data = private_data;
	fdpp[index]->timestamp = 0;
	number_of_filedescriptors++;
}

void remove_fd(SOCKET fd)
{
	uint32_t index=0;

	for (index = 0; index < number_of_filedescriptors; index++) {
		if (fds[index].fd == fd) break;
	}
	if (index == number_of_filedescriptors) {
		printlog(LOG_WARNING, "remove_fd : Couldn't find descriptor %d", fd);
	}
	else {
		struct pollplus* old = fdpp[index];
		TYPE2MGR(fdpp[index]->index)->cleanup(fdpp[index]->type, fds[index].fd, fdpp[index]->private_data);
		if (ISPRIO(fdpp[index]->type)) nprio--;
		memmove(&fds[index], &fds[index + 1], (number_of_filedescriptors - index - 1) * sizeof(struct pollfd));
		memmove(&fdpp[index], &fdpp[index + 1], (number_of_filedescriptors - index - 1) * sizeof(struct pollplus*));
		for (; index < number_of_filedescriptors; index++)
			fdperm[fds[index].fd] = index;
		free(old);
		number_of_filedescriptors--;
	}
}


void FileCleanUp()
{
	uint32_t index = 0;
	struct winvde_module* module;
	for (index = 0; index < number_of_filedescriptors; index++)
	{
		module = TYPE2MGR(fdpp[index]->index);
		if (module != NULL)
		{
			module->cleanup(fdpp[index]->type, fds[index].fd, fdpp[index]->private_data);
		}
	}
}

struct winvde_module* TYPE2MGR(char module_tag)
{
	struct winvde_module* ptr = winvde_modules;
	while (ptr && ptr->module_tag != module_tag) {
		ptr = ptr->next;
	}
	return ptr;
}