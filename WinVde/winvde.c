#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <process.h>
#include <memory.h>
#include <sys/stat.h>
#include <libwinvde.h>
#include <winvdeplugmodule.h>


#define STDSWITCH "\\.winvde2\\default"
#define OLDSTDSWITCH "\\.winvde2\\default.switch"

#define STD_LIB_PATH "\\lib:\\usr\\lib"
#define LIBWINVDEPLUG "libwinvdeplug_"
#define LIBWINVDEPLUGIN "\\winvdeplug\\" LIBWINVDEPLUG

#define DEFAULT_MODULE "winvde"

uint64_t __winvde_version_tag = 4;

char* strchrnul(const char* s, int c);

struct winvde_open_parameters
{
	char* winvde_args;
	char* description;
	int interface_version;
	struct winvde_open_args* winvde_open_args;
};

WINVDECONN* winvde_open_module_lib(const char* filename, struct winvde_open_parameters* parameters)
{
	WINVDECONN* conn = NULL;
	HMODULE library = LoadLibraryA(filename);
	if (library != NULL)
	{
		conn = (WINVDECONN*)malloc(sizeof(WINVDECONN));
		if (conn != NULL)
		{
			memset(conn, 0, sizeof(WINVDECONN));
			conn->module.winvde_open_real = (WINVDECONN * (__cdecl*)(const char*, const char*, int, struct winvde_open_args*))GetProcAddress(library, "winvde_vde_open");
			conn->module.winvde_recv = (size_t(__cdecl*)(WINVDECONN*, const char*, size_t, int))GetProcAddress(library, "winvde_vde_recv");
			conn->module.winvde_send = (size_t(__cdecl*)(WINVDECONN*, const char*, size_t, int))GetProcAddress(library, "winvde_vde_send");
			conn->module.winvde_datafiledescriptor = (int(__cdecl*)(WINVDECONN*))GetProcAddress(library, "winvde_vde_datafd");
			conn->module.winvde_ctlfiledescriptor = (int(__cdecl*)(WINVDECONN*))GetProcAddress(library, "winvde_vde_ctlfd");
			conn->module.winvde_close = (int(__cdecl*)(WINVDECONN*))GetProcAddress(library, "winvde_vde_close");
			if (
				conn->module.winvde_open_real &&
				conn->module.winvde_recv &&
				conn->module.winvde_send &&
				conn->module.winvde_datafiledescriptor &&
				conn->module.winvde_ctlfiledescriptor &&
				conn->module.winvde_close
			)
			{
				conn->handle = library;
				conn->opened = FALSE;
				return conn;
			}
			
		}
		// If we didnt return earlier, clean up
		if (conn){
			free(conn);
		}
		FreeLibrary(library);
	}
	return NULL;
}

WINVDECONN* winvde_open_samedir(const char* modname, const char* subdir, struct winvde_open_parameters* parameters)
{
	char path[MAX_PATH];
	char* slash = NULL;
	size_t len = 0;

	HMODULE hModule = NULL; // NULL to get the path of the current executable

	DWORD result = GetModuleFileNameA(hModule, path, MAX_PATH);
	if (result == 0) {
		fprintf(stderr, "Failed to get module file name. Error: %d\n", GetLastError() );
		return NULL;
	}

	fprintf(stdout,"The path of the current executable is: %s\n", path );
	
	slash = strrchr(path, '\\');
	if (slash)
	{
		len = slash - path;
		snprintf(path, MAX_PATH, "%*.*s%s%s.dll", (int)len, (int)len, path, subdir, modname);
		return winvde_open_module_lib(path, parameters);
	}
	errno = EPROTONOSUPPORT;
	return NULL;
}

WINVDECONN* winvde_open_dirlist(const char* modname, const char* dirlist, const char* subdir, struct winvde_open_parameters * parameters)
{
	char path[MAX_PATH];
	WINVDECONN* retval = NULL;
	if (dirlist)
	{
		while (*dirlist != 0)
		{
			int len = (int)(strchrnul(dirlist, ':') - dirlist);
			snprintf(path, MAX_PATH, "%*.*s%s%s.dll", len, len, dirlist, subdir, modname);
			retval = winvde_open_module_lib(path, parameters);
			if (retval != NULL || errno == EPROTONOSUPPORT)
			{
				return retval;
			}
			dirlist += len + (dirlist[len] == ':');
		}

	}
	errno = EPROTONOSUPPORT;
	return  NULL;
}

WINVDECONN* winvde_open_default(const char* modname, struct winvde_open_parameters* parameters)
{
	char path[MAX_PATH];
	snprintf(path, MAX_PATH, LIBWINVDEPLUG "%s.dll", modname);
	return winvde_open_module_lib(path, parameters);
}

WINVDECONN* winvde_open_module(const char* modname, struct winvde_open_parameters* parameters)
{
	WINVDECONN* retval = NULL;
	char* winvdeplugin_path = getenv("WINVDEPLUGIN_PATH");
	size_t winvde_args_length = strlen(parameters->winvde_args);
	char* winvde_args_copy = NULL;
	
	winvde_args_copy = (char*)malloc(winvde_args_length+1);
	if (winvde_args_copy != NULL)
	{
		strcpy_s(winvde_args_copy, winvde_args_length+1,parameters->winvde_args);
		parameters->winvde_args = winvde_args_copy;
		if (winvdeplugin_path)
		{
			return winvde_open_dirlist(modname, winvdeplugin_path, "\\" LIBWINVDEPLUG, parameters);
		}
		else
		{
			retval = winvde_open_samedir(modname,LIBWINVDEPLUGIN, parameters);
			if (retval != NULL || errno != EPROTONOSUPPORT) {
				return retval;
			}
			retval = winvde_open_samedir(modname, "\\" LIBWINVDEPLUGIN, parameters);
			if (retval != NULL || errno != EPROTONOSUPPORT)
			{
				retval->opened = FALSE;
				return retval;
			}
			// Dont use LD_LIBRARY_PATH in windows
			//retval = winvde_open_dirlist(modname, getenv("LD_LIBRARY_PATH"), LIBWINVDEPLUGIN, parameters);
			//if (retval != NULL || errno != EPROTONOSUPPORT) return retval;
			retval = winvde_open_dirlist(modname, STD_LIB_PATH, LIBWINVDEPLUGIN, parameters);
			if (retval != NULL || errno != EPROTONOSUPPORT)
			{
				retval->opened = FALSE;
				return retval;
			}
			return winvde_open_default(modname, parameters);
		}
	}
	else
	{
		fprintf(stderr, "winvde_open_module: parameters->winvde_args is NULL\n");
	}
	return NULL;
}

void set_newdescr(const char* description, char* newdescription, size_t newdescription_length)
{
	char* ssh_client = getenv("SSH_CLIENT");
	int pid = getpid();
	if (ssh_client)
	{
		int len = (int)(strchrnul(ssh_client, ' ') - ssh_client);
		snprintf(newdescription, newdescription_length, "%s pid=%d SSH=%*.*s", description, pid, len, len, ssh_client);
	}
	else
	{
		snprintf(newdescription, newdescription_length, "%s pid=%d", description, pid);
	}
}

WINVDECONN* winvde_open_real(const char* winvde_url, const char* description, int interface_version, struct winvde_open_args* open_args)
{
	WINVDECONN* conn = NULL;
	char std_vde_url[MAX_PATH];
	struct _stat64 statbuf;

	char newdescription[MAXDESCR];
	char* tag = NULL;
	int modlen = 0;
	char * modname = NULL;

	struct winvde_open_parameters parameters = {
		.description = newdescription,
		.interface_version = interface_version,
		.winvde_open_args = open_args
	};

	if (winvde_url == NULL)
	{
		winvde_url = "";
	}

	set_newdescr(description, newdescription, MAXDESCR);

	if (tag == NULL)
	{
		if (*winvde_url == '\0')
		{
			char* homedir = getenv("USERPROFILE");
			if (homedir)
			{
				snprintf(std_vde_url, MAX_PATH, "%s%s", homedir, STDSWITCH);
				if (_stat64(std_vde_url, &statbuf) >= 0)
				{
					winvde_url = std_vde_url;
				}
				else
				{
					snprintf(std_vde_url, MAX_PATH, "%s%s", homedir, OLDSTDSWITCH);
					if (_stat64(std_vde_url, &statbuf) >= 0)
					{
						winvde_url = std_vde_url;
					}
				}
			}
		}
		if (_stat64(winvde_url, &statbuf) >= 0)
		{
			if ((statbuf.st_mode & _S_IFREG) == _S_IFREG)
			{
				FILE* file = fopen(winvde_url, "r");
				if (file != NULL)
				{
					if (fgets(std_vde_url, MAX_PATH, file) != NULL)
					{
						std_vde_url[strlen(std_vde_url) - 1] = 0;
						winvde_url = std_vde_url;
					}
					tag = strstr(winvde_url, "://");
					fclose(file);
				}
			}
		}
	}
	if (winvde_url == NULL || tag == NULL)
	{
		parameters.winvde_args = (char*)winvde_url;
		return winvde_open_module(DEFAULT_MODULE, &parameters);
	}
	else
	{
		modlen = (int)(tag - winvde_url);
		modname = malloc(modlen + 1);
		if (modname)
		{
			snprintf(modname, modlen + 1, "%*.*s", modlen, modlen, winvde_url);
			parameters.winvde_args = (char*)winvde_url + (modlen + 3);
			conn = winvde_open_module(modname, &parameters);
			if (modname)
			{
				free(modname);
			}
			return conn;
		}
	}
	return NULL;
}

size_t winvde_recv(WINVDECONN* conn, const char * buff, size_t len, uint32_t flags)
{
	if (conn != NULL && conn->module.winvde_recv != NULL)
	{
		return conn->module.winvde_recv(conn, buff, len, flags);
	}
	else
	{
		errno = EBADF;
		return -1;
	}
	return 0;
}

size_t winvde_send(WINVDECONN* conn, const char * buff, size_t len, uint32_t flags)
{
	if (conn != NULL && conn->module.winvde_send != NULL)
	{
		return conn->module.winvde_send(conn, buff, len, flags);
	}
	else
	{
		errno = EBADF;
		return -1;
	}
	return 0;
}

int winvde_datafiledescriptor(WINVDECONN* conn)
{
	if (conn != NULL && conn->module.winvde_datafiledescriptor != NULL)
	{
		return conn->module.winvde_datafiledescriptor(conn);
	}
	else
	{
		errno = EBADF;
		return -1;
	}
	return 0;
}

int winvde_ctlfiledescriptor(WINVDECONN* conn)
{
	if (conn != NULL && conn->module.winvde_ctlfiledescriptor != NULL)
	{
		return conn->module.winvde_ctlfiledescriptor(conn);
	}
	else
	{
		errno = EBADF;
		return - 1;
	}
	return 0;
}

int winvde_close(WINVDECONN* conn)
{
	if (conn != NULL && conn->module.winvde_close != NULL)
	{
		HMODULE handle = conn->handle;
		// This should free itself
		int rv = conn->module.winvde_close(conn);
		if (handle != NULL)
		{
			FreeLibrary(handle);
		}
	}
	else
	{
		errno = EBADF;
		return -1;
	}
	return -1;
}

char * strchrnul(const char * s, int c)
{
	if (s != NULL)
	{
		while (*s != '\0')
		{
			if (*s == c)
			{

				break;
			}
			s++;
		}
	}
	return (char*)s;
}

