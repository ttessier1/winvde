#include <stdio.h>
#include <errno.h>

#include "winvde_plugin.h"

#if defined(VDEPLUGIN)


struct plugin* pluginh = NULL;
struct plugin** plugint = &pluginh;

int pluginlist(FILE* f, char* arg)
{
#define PLUGINFMT "%-22s %s"
	struct plugin* p;
	int rv = ENOENT;
	printoutc(f, PLUGINFMT, "NAME", "HELP");
	printoutc(f, PLUGINFMT, "------------", "----");
	for (p = pluginh; p != NULL; p = p->next) {
		if (strncmp(p->name, arg, strlen(arg)) == 0) {
			printoutc(f, PLUGINFMT, p->name, p->help);
			rv = 0;
		}
	}
	return rv;
}

/* This will be prefixed with getent("$HOME") */


#ifndef MAX
# define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
/*
 * Try to dlopen a plugin trying different names and locations:
 * (code from view-os by Gardenghi)
 *
 * 1) dlopen(modname)
 * 2) dlopen(modname.so)
 * 3) dlopen(user_umview_plugin_directory/modname)
 * 4) dlopen(user_umview_plugin_directory/modname.so)
 * 5) dlopen(global_umview_plugin_directory/modname)
 * 6) dlopen(global_umview_plugin_directory/modname.so)
 *
 */

#define TRY_DLOPEN(fmt...) \
{ \
	snprintf(testpath, tplen, fmt); \
	if ((handle = dlopen(testpath, flag))) \
	{ \
		free(testpath); \
		return handle; \
	} \
}

void* plugin_dlopen(const char* modname, int flag)
{
	void* handle;
	char* testpath;
	int tplen;
	char* homedir = getenv("HOME");

	if (!modname)
		return NULL;

	if ((handle = dlopen(modname, flag)))
		return handle;

	/* If there is no home directory, use / */
	if (!homedir)
		homedir = "/";

	tplen = strlen(modname) +
		strlen(MODULES_EXT) + 2 + // + 1 is for a '/' and + 1 for \0
		MAX(strlen(PLUGINS_DIR),
			strlen(homedir) + strlen(USER_PLUGINS_DIR));

	testpath = malloc(tplen);

	TRY_DLOPEN("%s%s", modname, MODULES_EXT);
	TRY_DLOPEN("%s%s/%s", homedir, USER_PLUGINS_DIR, modname);
	TRY_DLOPEN("%s%s/%s%s", homedir, USER_PLUGINS_DIR, modname, MODULES_EXT);
	TRY_DLOPEN("%s%s", PLUGINS_DIR, modname);
	TRY_DLOPEN("%s/%s%s", PLUGINS_DIR, modname, MODULES_EXT);

	free(testpath);
	return NULL;
}



int pluginadd(char* arg) {
	void* handle;
	struct plugin* p;
	int rv = ENOENT;
	if ((handle = plugin_dlopen(arg, RTLD_LAZY)) != NULL) {
		if ((p = (struct plugin*)dlsym(handle, "vde_plugin_data")) != NULL) {
			if (p->handle != NULL) { /* this dyn library is already loaded*/
				dlclose(handle);
				rv = EEXIST;
			}
			else {
				addplugin(p);
				p->handle = handle;
				rv = 0;
			}
		}
		else {
			rv = EINVAL;
		}
	}
	return rv;
}

int plugindel(char* arg) {
	struct plugin** p = &pluginh;
	while (*p != NULL) {
		void* handle;
		if (strncmp((*p)->name, arg, strlen(arg)) == 0
			&& ((*p)->handle != NULL)) {
			struct plugin* this = *p;
			delplugin(this);
			handle = this->handle;
			this->handle = NULL;
			dlclose(handle);
			return 0;
		}
		else
			p = &(*p)->next;
	}
	return ENOENT;
}
#endif
