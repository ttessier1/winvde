
#include <stdarg.h>
#include <stdint.h>
#include <errno.h>
#include <stdlib.h>
#include "winvde_debugcl.h"
#include "winvde_printfunc.h"
#include "winvde_ev.h"


//#ifdef DEBUGOPT

struct dbgcl* dbgclh = NULL;

struct dbgcl** dbgclt = &dbgclh;

void adddbgcl(int ncl, struct dbgcl* cl)
{
	uint32_t index = 0;
	if (ncl <= 0||!cl)
	{
		errno = EINVAL;
		return;
	}
	for (index = 0; index < (uint32_t)ncl; index++, cl++)
	{
		cl->next = NULL;
		(*dbgclt) = cl;
		dbgclt = (&cl->next);
	}
}

void deldbgcl(int ncl, struct dbgcl* cl)
{
	uint32_t index;
	if (ncl <= 0 || cl == NULL)
	{
		errno = EINVAL;
		return;
	}
	for (index = 0; index < (uint32_t)ncl; index++, cl++) {
		struct dbgcl** p = &dbgclh;
		while (*p != NULL) {
			if (*p == cl) {
				if (cl->fds) free(cl->fds);
				if (cl->fun) free(cl->fun);
				*p = cl->next;
			}
			else {
				p = &(*p)->next;
				dbgclt = p;
			}
		}
	}
}

static char _dbgnl = '\n';
void debugout(struct dbgcl* cl, const char* format, ...)
{
	va_list arg;
	unsigned short index = 0;
	char* dbg_header = NULL;
	char* dbg_msg = NULL;
	struct iovec iov[] = { {NULL,0},{NULL,0},{&_dbgnl,1} };

	va_start(arg, format);
	iov[0].iov_len = asprintf(&dbg_header, "3%03o %s ", cl->tag & 0777, cl->path);
	iov[0].iov_base = dbg_header;
	iov[1].iov_len = vasprintf(&dbg_msg, format, arg);
	iov[1].iov_base = dbg_msg;
	va_end(arg);

	for (index = 0; index < cl->nfds; index++)
	{
		writev(cl->fds[index], iov, 3);
	}
	free(dbg_header);
	free(dbg_msg);
}

void eventout(struct dbgcl* cl, ...)
{
	unsigned short index;
	va_list arg;
	for (index = 0; index < cl->nfun; index++)
	{
		va_start(arg, cl);
		(cl->fun[index])(cl, cl->funarg[index], arg);
		va_end(arg);
	}
}

int packetfilter(struct dbgcl* cl, ...)
{
	uint32_t index;
	va_list arg;
	int len;
	int rv;

	va_start(arg, cl);
	(void)va_arg(arg, int); /*port*/
	(void)va_arg(arg, char*); /*buf*/
	len = va_arg(arg, int);
	va_end(arg);

	for (index = 0; index < cl->nfun && len>0; index++)
	{
		va_start(arg, cl);
		rv = (cl->fun[index])(cl, cl->funarg[index], arg);
		va_end(arg);
		if (rv != 0)
		{
			len = rv;
		}
	}
	if (len < 0)
	{
		return 0;
	}
	else
	{
		return len;
	}
}

//#endif
