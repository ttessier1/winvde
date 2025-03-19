#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "winvde_comlist.h"

struct comlist* clh = NULL;
struct comlist** clt = &clh;

void addcl(int ncl, struct comlist* cl)
{
	uint32_t index=0;
	for (index = 0; index < ncl; index++, cl++)
	{
		cl->next = NULL;
		(*clt) = cl;
		clt = (&cl->next);
	}
}

void delcl(int ncl, struct comlist* cl)
{
	uint32_t index = 0;
	for (index = 0; index < ncl; index++, cl++) {
		struct comlist** p = &clh;
		while (*p != NULL) {
			if (*p == cl)
				*p = cl->next;
			else {
				p = &(*p)->next;
				clt = p;
			}
		}
	}
}