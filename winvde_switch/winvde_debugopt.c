#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "winvde_debugopt.h"
#include "winvde_output.h"
#include "winvde_debugcl.h"
#include "winvde_comlist.h"

#define DEBUGFORMAT1 "%-22s %-3s %-6s %s"
#define DEBUGFORMAT2 "%-22s %03o %-6s %s"

#if defined(DEBUGOPT)
//int debuglist(FILE* f, int fd, char* path)
int debuglist(struct comparameter * parameter)
{
	FILE* file_stream = NULL;
	SOCKET socket_stream = INVALID_SOCKET;
	struct dbgcl* p;
	int i;
	int rv = ENOENT;
	if (!parameter)
	{
		errno = EINVAL;
		return -1;
	}
	if (parameter->type1 == com_type_file && parameter->data1.file_descriptor != NULL && parameter->paramType == com_param_type_string && parameter->paramValue != NULL)
	{
		file_stream = parameter->data1.file_descriptor;
		printoutc(file_stream, DEBUGFORMAT1, "CATEGORY", "TAG", "STATUS", "HELP");
		printoutc(file_stream, DEBUGFORMAT1, "------------", "---", "------", "----");
		for (p = dbgclh; p != NULL; p = p->next) {
			if (p->help && strncmp(p->path, parameter->paramValue.stringValue, strlen(parameter->paramValue.stringValue)) == 0) {
				for (i = 0; i < p->nfds && p->fds[i] != file_stream; i++)
				{
					;
				}
				rv = 0;
				printoutc(file_stream, DEBUGFORMAT2, p->path, p->tag & 0777, i < p->nfds ? "ON" : "OFF", p->help);
			}
		}
	}
	return rv;
}

/* EINVAL -> no matches
 * EEXIST -> all the matches already include fd
 * ENOMEM -> fd buffer realloc failed
 * 0 otherwise */
int debugadd(struct comparameter* parameter){
//int debugadd(int fd, char* path) {
	struct dbgcl* p;
	int rv = EINVAL;
	int index;
	if (!parameter)
	{
		errno = EINVAL;
		return -1;
	}
	if (parameter->type1 == com_type_file && parameter->data1.file_descriptor != NULL && parameter->paramType == com_param_type_string && parameter->paramValue != NULL)
	{
		for (p = dbgclh; p != NULL; p = p->next) {
			if (p->help && strncmp(p->path, parameter->paramValue.stringValue, strlen(parameter->paramValue.stringValue)) == 0) {
				
				if (rv == EINVAL) rv = EEXIST;
				for (index = 0; index < p->nfds && (p->fds[index] != fd); index++)
				{
					;
				}
				if (index >= p->nfds) {
					if (index >= p->maxfds) {
						int newsize = p->maxfds + DBGCLSTEP;
						p->fds = realloc(p->fds, newsize * sizeof(int));
						if (p->fds) {
							p->maxfds = newsize;
							p->fds[index] = fd;
							p->nfds++;
							if (rv != ENOMEM) rv = 0;
						}
						else
							rv = ENOMEM;
					}
					else {
						p->fds[index] = fd;
						p->nfds++;
						if (rv != ENOMEM) rv = 0;
					}
				}
			}
		}
	}
	return rv;
}

/* EINVAL -> no matches
 * ENOENT -> all the matches do not include fd
 * 0 otherwise */
int debugdel(struct comparameter* parameter) {
//int debugdel(int fd, char* path) {
	struct dbgcl* p;
	int rv = EINVAL;
	if (!parameter)
	{
		errno = EINVAL;
		return -1;
	}
	if (
		parameter->type1 == com_type_file && 
		parameter->paramType == com_param_type_string &&
		parameter->paramValue.stringValue != NULL	
	)
	{
		for (p = dbgclh; p != NULL; p = p->next) {
			if (strncmp(p->path, parameter->paramValue.stringValue, strlen(parameter->paramValue.stringValue)) == 0) {
				int i;
				if (rv == EINVAL) rv = ENOENT;
				for (i = 0; i < p->nfds && (p->fds[i] != fd); i++)
				{
					;
				}
				if (i < p->nfds) {
					p->nfds--; /* the last one */
					p->fds[i] = p->fds[p->nfds]; /* swap it with the deleted element*/
					rv = 0;
				}
			}
		}
	}
	return rv;
}

void debugout(struct dbgcl* cl, const char* format, ...)
{
	va_list arg;
	char* msg;
	int i;
	char* header;
	struct iovec iov[] = { {NULL,0},{NULL,0},{&_dbgnl,1} };

	va_start(arg, format);
	iov[0].iov_len = asprintf(&header, "3%03o %s ", cl->tag & 0777, cl->path);
	iov[0].iov_base = header;
	iov[1].iov_len = vasprintf(&msg, format, arg);
	iov[1].iov_base = msg;
	va_end(arg);

	for (i = 0; i < cl->nfds; i++)
		writev(cl->fds[i], iov, 3);
	free(header);
	free(msg);
}

int eventadd(int (*fun)(), char* path, void* arg) {
	struct dbgcl* p;
	int rv = EINVAL;
	for (p = dbgclh; p != NULL; p = p->next) {
		if (strncmp(p->path, path, strlen(path)) == 0) {
			int i;
			if (rv == EINVAL) rv = EEXIST;
			for (i = 0; i < p->nfun && (p->fun[i] != fun); i++)
				;
			if (i >= p->nfun) {
				if (i >= p->maxfun) {
					int newsize = p->maxfun + DBGCLSTEP;
					p->fun = realloc(p->fun, newsize * sizeof(int));
					p->funarg = realloc(p->funarg, newsize * sizeof(void*));
					if (p->fun && p->funarg) {
						p->maxfun = newsize;
						p->fun[i] = fun;
						p->funarg[i] = arg;
						p->nfun++;
						if (rv != ENOMEM) rv = 0;
					}
					else
						rv = ENOMEM;
				}
				else {
					p->fun[i] = fun;
					p->nfun++;
					if (rv != ENOMEM) rv = 0;
				}
			}
		}
	}
	return rv;
}

/* EINVAL -> no matches
 * ENOENT -> all the matches do not include fun
 * 0 otherwise */
int eventdel(int (*fun)(), char* path, void* arg) {
	struct dbgcl* p;
	int rv = EINVAL;
	for (p = dbgclh; p != NULL; p = p->next) {
		if (strncmp(p->path, path, strlen(path)) == 0) {
			int i;
			if (rv == EINVAL) rv = ENOENT;
			for (i = 0; i < p->nfun && (p->fun[i] != fun) && (p->funarg[i] != arg); i++)
				;
			if (i < p->nfun) {
				p->nfun--; /* the last one */
				p->fun[i] = p->fun[p->nfun]; /* swap it with the deleted element*/
				rv = 0;
			}
		}
	}
	return rv;
}

void eventout(struct dbgcl* cl, ...)
{
	int i;
	va_list arg;
	for (i = 0; i < cl->nfun; i++) {
		va_start(arg, cl);
		(cl->fun[i])(cl, cl->funarg[i], arg);
		va_end(arg);
	}
}

#endif