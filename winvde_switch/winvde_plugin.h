#pragma once

#include <stdio.h>


#if defined(VDEPLUGIN)

#define USER_PLUGINS_DIR ".winvde2\\plugins"

struct plugin {
	char* name;
	char* help;
	void* handle;
	struct plugin* next;
};

extern struct plugin* pluginh;
extern struct plugin** plugint;



int pluginlist(struct comparameter* parameter);
HMODULE plugin_dlopen(const char* modname);
int pluginadd(struct comparameter * parameter);
int plugindel(struct comparameter* parameter);
void addplugin(struct plugin* cl);
void delplugin(struct plugin* cl);
#else

#define TRY_DLOPEN(fmt,...) 

#endif