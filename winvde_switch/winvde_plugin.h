#pragma once

#include <stdio.h>

#if defined(VDEPLUGIN)

#define USER_PLUGINS_DIR "%USERPROFILE%\\.winvde2\\plugins"

extern struct plugin* pluginh;
extern struct plugin** plugint;


static int pluginlist(FILE* f, char* arg);
void* plugin_dlopen(const char* modname, int flag);
static int pluginadd(char* arg);
static int plugindel(char* arg);

#endif