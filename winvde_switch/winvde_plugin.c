#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <vadefs.h>
#include <stdarg.h>
#include <errno.h>

#include "winvde_plugin.h"
#include "winvde_comlist.h"
#include "winvde_output.h"
#include "winvde_printfunc.h"
#include "winvde_memorystream.h"

#if defined(VDEPLUGIN)


#ifndef MAX
# define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#define MODULES_EXT ".dll"
#define PLUGINS_DIR ".winvde2\\plugins"

struct plugin* pluginh = NULL;
struct plugin** plugint = &pluginh;

//int pluginlist(FILE* f, char* arg)
int pluginlist(struct comparameter * parameter)
{
	char* tmpBuff = NULL;
	size_t length = 0;
#define PLUGINFMT "%-30s %-50s\n"
	struct plugin* plugin_struct;
	int rv = ENOENT;
	if (!parameter)
	{
		errno = EINVAL;
		return -1;
	}
	if (parameter->type1 == com_type_file &&
		parameter->data1.file_descriptor != NULL &&
		parameter->paramType == com_param_type_string &&
		parameter->paramValue.stringValue != NULL
		)
	{
		printoutc(parameter->data1.file_descriptor, PLUGINFMT, "NAME", "HELP");
		printoutc(parameter->data1.file_descriptor, PLUGINFMT, "------------------------------", "----------------------------------------");
		for (plugin_struct = pluginh; plugin_struct != NULL; plugin_struct = plugin_struct->next)
		{
			if (strncmp(plugin_struct->name, parameter->paramValue.stringValue, strlen(parameter->paramValue.stringValue)) == 0)
			{
				printoutc(parameter->data1.file_descriptor, PLUGINFMT, plugin_struct->name, plugin_struct->help);
				rv = 0;
			}
		}
		return rv;
	}
	else if (parameter->type1 == com_type_socket &&
		parameter->data1.socket != INVALID_SOCKET &&
		parameter->paramType == com_param_type_string &&
		parameter->paramValue.stringValue != NULL)
	{
		length = asprintf(&tmpBuff, PLUGINFMT, "NAME", "HELP");
		if (length > 0 && tmpBuff)
		{
			send(parameter->data1.socket, tmpBuff, (int)length,0);
			free(tmpBuff);
		}
		else
		{
			errno = ENOMEM;
			return -1;
		}
		length = asprintf(&tmpBuff, PLUGINFMT, "------------------------------", "----------------------------------------");
		if (length > 0 && tmpBuff)
		{
			send(parameter->data1.socket, tmpBuff, (int)length,0);
			free(tmpBuff);
		}
		else
		{
			errno = ENOMEM;
			return -1;
		}

		for (plugin_struct = pluginh; plugin_struct != NULL; plugin_struct = plugin_struct->next)
		{
			if (strncmp(plugin_struct->name, parameter->paramValue.stringValue, strlen(parameter->paramValue.stringValue)) == 0)
			{
				length = asprintf(&tmpBuff, PLUGINFMT, plugin_struct->name, plugin_struct->help);
				if (length > 0 && tmpBuff)
				{
					send(parameter->data1.socket, tmpBuff, (int)length,0);
					free(tmpBuff);
				}
				else
				{
					errno = ENOMEM;
					return -1;
				}
				rv = 0;
			}
		}
		return rv;
	}
	else if (parameter->type1 == com_type_memstream &&
		parameter->data1.mem_stream != NULL &&
		parameter->paramType == com_param_type_string &&
		parameter->paramValue.stringValue != NULL)
	{
		length = asprintf(&tmpBuff, PLUGINFMT, "NAME", "HELP");
		if (length > 0 && tmpBuff)
		{
			write_memorystream(parameter->data1.mem_stream, tmpBuff, length);
			free(tmpBuff);
		}
		else
		{
			errno = ENOMEM;
			return -1;
		}
		length = asprintf(&tmpBuff, PLUGINFMT, "------------------------------", "----------------------------------------");
		if (length > 0 && tmpBuff)
		{
			write_memorystream(parameter->data1.mem_stream, tmpBuff, length);
			free(tmpBuff);
		}
		else
		{
			errno = ENOMEM;
			return -1;
		}
		
		for (plugin_struct = pluginh; plugin_struct != NULL; plugin_struct = plugin_struct->next)
		{
			if (strncmp(plugin_struct->name, parameter->paramValue.stringValue, strlen(parameter->paramValue.stringValue)) == 0)
			{
				length = asprintf(&tmpBuff, PLUGINFMT, plugin_struct->name, plugin_struct->help);
				if (length > 0 && tmpBuff)
				{
					write_memorystream(parameter->data1.mem_stream, tmpBuff, length);
					free(tmpBuff);
				}
				else
				{
					errno = ENOMEM;
					return -1;
				}
				rv = 0;
			}
		}
		return rv;
	}
	else
	{
		errno = EINVAL;
		return -1;
	}
}


HMODULE plugin_dlopen(const char* modname)
{
	HMODULE handle = INVALID_HANDLE_VALUE;
	char* testpath=NULL;
	size_t tplen = 0;
	char homedir[MAX_PATH];
	size_t count = 0;
	if (!modname)
	{
		return NULL;
	}
	handle = LoadLibraryA(modname);
	if (handle != NULL)
	{
		return handle;
	}

	count = MAX_PATH;
	getenv_s(&count,homedir,MAX_PATH,"USERPROFILE");

	/* If there is no home directory, use / */
	if (!homedir)
	{
		strncpy_s(homedir,MAX_PATH,"\\",strlen("\\"));
	}
	else
	{
		strncat_s(homedir, MAX_PATH, "\\", 1);
	}

	tplen = strlen(modname) +
		strlen(MODULES_EXT) + 2 + // + 1 is for a '/' and + 1 for \0
		MAX(strlen(PLUGINS_DIR),
		strlen(homedir) + strlen(USER_PLUGINS_DIR) + 1);

	testpath = (char*)malloc(tplen);
	if (testpath)
	{
		if ((handle = TRY_DLOPEN(testpath, tplen, "%s%s", modname, MODULES_EXT)) != INVALID_HANDLE_VALUE) { return handle; }
		if ((handle = TRY_DLOPEN(testpath, tplen, "%s%s\\%s", homedir, USER_PLUGINS_DIR, modname)) != INVALID_HANDLE_VALUE) { return handle; }
		if ((handle = TRY_DLOPEN(testpath, tplen, "%s%s\\%s%s", homedir, USER_PLUGINS_DIR, modname, MODULES_EXT)) != INVALID_HANDLE_VALUE) { return handle; }
		if ((handle = TRY_DLOPEN(testpath, tplen, "%s%s", PLUGINS_DIR, modname)) != INVALID_HANDLE_VALUE) { return handle; }
		if ((handle = TRY_DLOPEN(testpath, tplen, "%s\\%s%s", PLUGINS_DIR, modname, MODULES_EXT)) != INVALID_HANDLE_VALUE) { return handle; }
		if (testpath)
		{
			free(testpath);
		}
	}
	return NULL;
}



//int pluginadd(char* arg) {
int pluginadd(struct comparameter * parameter)
{
	HMODULE handle = INVALID_HANDLE_VALUE;
	struct plugin* plugin_struct = NULL;
	int rv = ENOENT;
	if(!parameter)
	{
		errno = EINVAL;
		return -1;
	}
	if (parameter->type1 == com_type_null && 
		parameter->paramType == com_param_type_string && 
		parameter->paramValue.stringValue != NULL
	)
	{
		if ((handle = plugin_dlopen(parameter->paramValue.stringValue)) != NULL)
		{
			plugin_struct = (struct plugin*)GetProcAddress(handle, "winvde_plugin_data");
			if (plugin_struct != NULL)
			{
				if (strstr(parameter->paramValue.stringValue, "winvde_") == NULL)
				{
					fprintf(stderr, "Plugin name does not match plugin name expectation\n");
					FreeLibrary(handle);
					errno = EINVAL;
					rv = EINVAL;
				}
				if (strcmp(plugin_struct->name, parameter->paramValue.stringValue) != 0)
				{
					fprintf(stderr, "Plugin name does not match plugin name requested\n");
					FreeLibrary(handle);
					errno = EINVAL;
					rv = EINVAL;
				}
				else
				{
					if (plugin_struct->handle != NULL)
					{ /* this dyn library is already loaded*/
						FreeLibrary(handle);
						rv = EEXIST;
					}
					else
					{
						addplugin(plugin_struct);
						plugin_struct->handle = handle;
						rv = 0;
					}
				}
			}
			else
			{
				rv = EINVAL;
			}
		}
		return rv;
	}
	else
	{
		errno = EINVAL;
		return -1;
	}
}

//int plugindel(char* arg) {
int plugindel(struct comparameter * parameter) {
	struct plugin** p = &pluginh;
	HMODULE handle;
	struct plugin* _this = NULL;
	if (!parameter || !p)
	{
		errno = EINVAL;
		return -1;
	}
	if (parameter->type1 == com_type_null &&
		parameter->paramType == com_param_type_string &&
		parameter->paramValue.stringValue != NULL
		)
	{
		while (*p != NULL)
		{
			if (
				strncmp((*p)->name, parameter->paramValue.stringValue, strlen(parameter->paramValue.stringValue)) == 0
				&& ((*p)->handle != NULL))
			{
				_this = *p;
				delplugin(_this);
				handle = _this->handle;
				_this->handle = NULL;
				FreeLibrary(handle);
				return 0;
			}
			else
			{
				p = &(*p)->next;
			}
		}
		return ENOENT;
	}
	else
	{
		errno = EINVAL;
		return -1;
	}
}

void addplugin(struct plugin* cl)
{
	if (!cl)
	{
		return;
	}
	cl->next = NULL;
	(*plugint) = cl;
	plugint = (&cl->next);
}

void delplugin(struct plugin* cl)
{
	struct plugin** p = plugint = &pluginh;
	if(!cl||!p)
	{
		return;
	}
	while (*p != NULL)
	{
		if (*p == cl)
		{
			*p = cl->next;
		}
		else
		{
			p = &(*p)->next;
			plugint = p;
		}
	}
}

int TRY_DLOPEN(char* testpath, int tplen, char* fmt, ...)
{
	HANDLE handle = NULL;
	va_list args;

	va_start(args, fmt);
	vsnprintf(testpath, tplen, fmt, args);
	va_end(args);
	handle = LoadLibraryA(testpath);
	if (handle != NULL)
	{
		free(testpath);
		return handle;
	}
	return NULL;
}

#endif
