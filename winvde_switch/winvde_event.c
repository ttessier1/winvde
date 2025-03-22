
#include <errno.h>

#include "winvde_event.h"
#include "winvde_debugcl.h"

#if defined(DEBUGOPT)

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