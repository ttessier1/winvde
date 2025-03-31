#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>

#include "winvde_debugcl.h"
#include "winvde_debugopt.h"
#include "winvde_output.h"
#include "winvde_comlist.h"
#include "winvde_ev.h"
#include "winvde_printfunc.h"
#include "winvde_event.h"
#include "winvde_sockutils.h"
#include "winvde_memorystream.h"
#include "winvde_mgmt.h"


#if defined(DEBUGOPT)

char _dbgnl = '\n';

//int debuglist(FILE* f, int fd, char* path)
int debuglist(struct comparameter* parameter)
{
	size_t length = 0;
	char* tmpBuff = NULL;
	SOCKET socket_stream = INVALID_SOCKET;
	struct dbgcl* p;
	int index;
	int rv = ENOENT;
#define DEBUGFORMAT1 "%-22s %-3s %-6s %s\n"
#define DEBUGFORMAT2 "%-22s %03o %-6s %s\n"
	
	if (!parameter)
	{
		switch_errno = EINVAL;
		return -1;
	}
	if (parameter->type1 == com_type_socket && 
		parameter->data1.socket != INVALID_SOCKET && 
		parameter->type2 == com_type_socket &&
		parameter->data2.socket != INVALID_SOCKET &&
		parameter->paramType == com_param_type_string && 
		parameter->paramValue.stringValue != NULL)
	{
		length = asprintf(&tmpBuff, DEBUGFORMAT1, "CATEGORY", "TAG", "STATUS", "HELP");
		if (length > 0 && tmpBuff)
		{
			send(parameter->data1.socket, tmpBuff, (int)length, 0);
			free(tmpBuff);
		}
		else
		{
			switch_errno = ENOMEM;
			return -1;
		}
		length = asprintf(&tmpBuff, DEBUGFORMAT1, "------------", "---", "------", "----");
		if (length > 0 && tmpBuff)
		{
			send(parameter->data1.socket, tmpBuff, (int)length, 0);
			free(tmpBuff);
		}
		else
		{
			switch_errno = ENOMEM;
			return -1;
		}
		for (p = dbg_cl_header; p != NULL; p = p->next)
		{
			if (p->help && strncmp(p->path, parameter->paramValue.stringValue, strlength(parameter->paramValue.stringValue)) == 0)
			{
				for (index = 0; index < p->nfds && p->fds[index] != parameter->data2.socket; index++)
				{
					;
				}
				rv = 0;
				length = asprintf(&tmpBuff, DEBUGFORMAT2, p->path, p->tag & 0777, index < p->nfds ? "ON" : "OFF", p->help);
				if (length > 0 && tmpBuff)
				{
					send(parameter->data1.socket, tmpBuff, (int)length, 0);
					free(tmpBuff);
				}
				else
				{
					switch_errno = ENOMEM;
					return -1;
				}
			}
		}
	}
	else if (parameter->type1 == com_type_memstream && 
		parameter->data1.mem_stream != NULL && 
		parameter->type2 == com_type_socket &&
		parameter->data2.socket != INVALID_SOCKET &&
		parameter->paramType == com_param_type_string && 
		parameter->paramValue.stringValue != NULL)
	{
		length = asprintf(&tmpBuff, DEBUGFORMAT1, "CATEGORY", "TAG", "STATUS", "HELP");
		if (length > 0 && tmpBuff)
		{
			write_memorystream(parameter->data1.mem_stream, tmpBuff, length);
			free(tmpBuff);
		}
		else
		{
			switch_errno = ENOMEM;
			return -1;
		}
		length = asprintf(&tmpBuff, DEBUGFORMAT1, "------------", "---", "------", "----");
		if (length > 0 && tmpBuff)
		{
			write_memorystream(parameter->data1.mem_stream, tmpBuff, length);
			free(tmpBuff);
		}
		else
		{
			switch_errno = ENOMEM;
			return -1;
		}
		for (p = dbg_cl_header; p != NULL; p = p->next)
		{
			if (p->help && strncmp(p->path, parameter->paramValue.stringValue, strlength(parameter->paramValue.stringValue)) == 0)
			{
				for (index = 0; index < p->nfds && p->fds[index] != parameter->data2.socket; index++)
				{
					;
				}
				rv = 0;
				length = asprintf(&tmpBuff, DEBUGFORMAT2, p->path, p->tag & 0777, index < p->nfds ? "ON" : "OFF", p->help);
				if (length > 0 && tmpBuff)
				{
					write_memorystream(parameter->data1.mem_stream, tmpBuff, length);
					free(tmpBuff);
				}
				else
				{
					switch_errno = ENOMEM;
					return -1;
				}
			}
		}
	}
	return rv;
}

int debugadd(struct comparameter* parameter)
{
	struct dbgcl* p = NULL;
	int rv = EINVAL;
	int index;
	if (!parameter)
	{
		switch_errno = EINVAL;
		return -1;
	}
	if (
		parameter->type1 == com_type_socket &&
		parameter->data1.socket != INVALID_SOCKET &&
		parameter->paramType == com_param_type_string &&
		parameter->paramValue.stringValue != NULL
		)
	{
		for (p = dbg_cl_header; p != NULL; p = p->next)
		{
			if (p->help && strncmp(p->path, parameter->paramValue.stringValue, strlen(parameter->paramValue.stringValue)) == 0) {

				if (rv == EINVAL)
				{
					rv = EEXIST;
				}
				for (index = 0; index < p->nfds && (p->fds[index] != parameter->data1.socket); index++)
				{
					;
				}
				if (index >= p->nfds)
				{
					if (index >= p->maxfds)
					{
						int newsize = p->maxfds + DBGCLSTEP;
						p->fds = (SOCKET*)realloc(p->fds, newsize * sizeof(int));
						if (p->fds)
						{
							p->maxfds = newsize;
							p->fds[index] = parameter->data1.socket;
							p->nfds++;
							if (rv != ENOMEM) rv = 0;
						}
						else
						{
							rv = ENOMEM;
						}
					}
					else
					{
						p->fds[index] = parameter->data1.socket;
						p->nfds++;
						if (rv != ENOMEM)
						{
							rv = 0;
						}
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
	struct dbgcl* p;
	int rv = EINVAL;
	if (!parameter)
	{
		switch_errno = EINVAL;
		return -1;
	}
	if (
		parameter->type1 == com_type_socket &&
		parameter->data1.socket != INVALID_SOCKET &&
		parameter->paramType == com_param_type_string &&
		parameter->paramValue.stringValue != NULL
		)
	{
		for (p = dbg_cl_header; p != NULL; p = p->next) {
			if (strncmp(p->path, parameter->paramValue.stringValue, strlen(parameter->paramValue.stringValue)) == 0) {
				int i;
				if (rv == EINVAL) rv = ENOENT;
				for (i = 0; i < p->nfds && (p->fds[i] != parameter->data1.socket); i++)
				{
					;
				}
				if (i < p->nfds) {
					p->nfds--;
					p->fds[i] = p->fds[p->nfds];
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
	{
		writev(cl->fds[i], iov, 3);
	}
	free(header);
	free(msg);
}

int packet_filter(struct dbgcl* cl, unsigned short port, char* buff, int length)
{
	unsigned short index = 0;
	int returnValue = 0;
	if (!cl)
	{
		switch_errno = EINVAL;
		return -1;
	}
	for (index = 0; index < cl->nfun && length>0; index++)
	{
		//va_start(arg, cl);
		if (cl->fun != NULL)
		{
			returnValue = (cl->fun[index])(cl);
		}
		//va_end(arg);
		if (returnValue != 0)
		{
			length = returnValue;
		}
	}
	if (length < 0)
	{
		return 0;
	}
	else
	{
		return length;
	}
}

#endif