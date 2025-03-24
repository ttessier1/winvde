
#include <stdarg.h>
#include <stdint.h>
#include <errno.h>
#include <stdlib.h>

#include "winvde_debugcl.h"
#include "winvde_printfunc.h"
#include "winvde_ev.h"

#if defined(DEBUGOPT)

struct dbgcl* dbg_cl_header = NULL;

struct dbgcl** dbgclt = &dbg_cl_header;

void adddbgcl(size_t ncl, struct dbgcl* cl)
{
	size_t index = 0;
	if (ncl <= 0||!cl)
	{
		errno = EINVAL;
		return;
	}
	for (index = 0; index < ncl; index++, cl++)
	{
		cl->next = NULL;
		(*dbgclt) = cl;
		dbgclt = (&cl->next);
	}
}

void deldbgcl(size_t ncl, struct dbgcl* cl)
{
	size_t index;
	struct dbgcl** p = NULL;
	if (ncl <= 0 || cl == NULL)
	{
		errno = EINVAL;
		return;
	}
	for (index = 0; index < (uint32_t)ncl; index++, cl++)
	{
		p = &dbg_cl_header;
		while (*p != NULL)
		{
			if (*p == cl)
			{
				if (cl->fds!=NULL)
				{
					free(cl->fds);
				}
				if (cl->fun!=NULL)
				{
					free(cl->fun);
				}
				*p = cl->next;
			}
			else
			{
				p = &(*p)->next;
				dbgclt = p;
			}
		}
	}
}
#endif
