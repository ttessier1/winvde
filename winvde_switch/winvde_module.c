#include "winvde_module.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <direct.h>

struct winvde_module* winvde_modules;

static void recaddmodule(struct winvde_module** p, struct winvde_module* new)
{
	struct winvde_module* _this = *p;
	if (_this == NULL)
	{
		*p = new;
	}
	else
	{
		recaddmodule(&(_this->next), new);
	}
}

void add_module(struct winvde_module* new)
{
	static int lastlwmtag;
	new->module_tag = ++lastlwmtag;
	if (new != NULL && new->module_tag != 0) {
		new->next = NULL;
		recaddmodule(&winvde_modules, new);
	}
}

static void recdelswm(struct winvde_module** p, struct winvde_module* old)
{
	struct winvde_module* this = *p;
	if (this != NULL) {
		if (this == old)
			*p = this->next;
		else
			recdelswm(&(this->next), old);
	}
}

void del_module(struct winvde_module* old)
{
	if (old != NULL) {
		recdelswm(&winvde_modules, old);
	}
}

void init_mods(void)
{
	struct winvde_module * module;
	char* path[MAX_PATH];
	/* Keep track of the initial cwd */
	if (_getcwd((char*)path, sizeof(path)) != NULL)
	{

		for (module = winvde_modules; module != NULL; module = module->next)
		{
			if (module->init != NULL)
			{
				module->init();
				// reset folder if we were changed by module
				if (_chdir((char*)path)>0)
				{
					fprintf(stderr, "Failed to set the current directory\n");
				}
			}
		}
	}
}

