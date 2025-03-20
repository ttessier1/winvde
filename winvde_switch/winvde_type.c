#include <winmeta.h>

#include <errno.h>
#include <stdlib.h>
#include <memory.h>

#include "winvde_loglevel.h"
#include "winvde_type.h"
#include "winvde_module.h"
#include "winvde_switch.h"
#include "winvde_output.h"

struct winvde_module** fdtypes;

int ntypes;
int maxtypes;

unsigned char add_type(struct winvde_module* mgr, int prio)
{
	struct winvde_module* ptr = NULL;
	uint32_t index=0;
	if (ntypes == maxtypes) {
		maxtypes = maxtypes ? 2 * maxtypes : 8;
		if (maxtypes > PRIOFLAG) {
			printlog(LOG_ERR, "too many file types");
			exit(1);
		}
		if ((fdtypes = (struct winvde_module**)realloc(fdtypes, maxtypes * sizeof(struct winvde_module*))) == NULL) {
			strerror_s(errorbuff,sizeof(errorbuff),errno);
			printlog(LOG_ERR, "realloc fdtypes %s", errorbuff);
			exit(1);
		}
		memset(fdtypes + ntypes, 0, sizeof(struct winvde_module*) * maxtypes - ntypes);
		index = ntypes;
	}
	else
	{
		for (index = 0; fdtypes[index] != NULL; index++)
		{
			;
		}
	}
	fdtypes[index] = mgr;
	
	ptr = mgr;
	ntypes++;
	return index | ((prio != 0) ? PRIOFLAG : 0);
}

void del_type(uint8_t type)
{
	type &= TYPEMASK;
	if (type < maxtypes)
	{
		fdtypes[type] = NULL;
	}
	ntypes--;
}
