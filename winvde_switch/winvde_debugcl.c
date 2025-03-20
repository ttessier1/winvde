
#include <stdarg.h>
#include <stdint.h>
#include "winvde_debugcl.h"

//#ifdef DEBUGOPT

struct dbgcl* dbgclh = NULL;

struct dbgcl** dbgclt = &dbgclh;

void adddbgcl(int ncl, struct dbgcl* cl)
{
	uint32_t index = 0;
	for (index = 0; index < ncl; index++, cl++)
	{
		cl->next = NULL;
		(*dbgclt) = cl;
		dbgclt = (&cl->next);
	}
}

void deldbgcl(int ncl, struct dbgcl* cl)
{
	uint32_t index;
	for (index = 0; index < ncl; index++, cl++) {
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
