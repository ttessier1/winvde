#define WIN32_LEAN_AND_MEA
#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

//#include "vde.h"
//#include "vdecommon.h"

#include "winvde_plugin.h"

#define DUMP_FILE "winvde_pkt.dmp"


static int testevent(struct dbgcl* tag, void* arg, va_list v);
static int dump(char* arg);

__declspec(dllexport ) struct plugin winvde_plugin_data = {
	.name = "winvde_dump",
	.help = "dump packets",
};

struct comlist cl[] = {
	{"dump","============","DUMP Packets",NULL,NOARG},
	{"dump/active","0/1","start dumping data",dump,STRARG},
};

#define D_DUMP 0100 
struct dbgcl dl[] = {
	 {"dump/packetin","dump incoming packet",D_DUMP | D_IN},
	 {"dump/packetout","dump outgoing packet",D_DUMP | D_OUT},
};

int dump(char* arg)
{
	int active = atoi(arg);
	if (active)
	{
		eventadd(testevent, "packet", dl);
	}
	else
	{
		eventdel(testevent, "packet", dl);
	}
	return 0;
}

int testevent(struct dbgcl* event, void* arg, va_list v)
{
	struct dbgcl* this = arg;
	int port = 0;
	unsigned char* buf = NULL;
	int len = 0;
	int index=0;
	char* pktdump = NULL;
	size_t dumplen = 0;
	struct _stat64 filestat;
	FILE* out = NULL;
	switch (event->tag) {
		case D_PACKET | D_OUT:
			this++;
		case D_PACKET | D_IN:
			{
				port = va_arg(v, int);
				buf = va_arg(v, unsigned char*);
				len = va_arg(v, int);
				pktdump = NULL;
				dumplen = 0;
				if (_stat64(DUMP_FILE, &filestat)==0)
				{
					if (fopen_s(&out, DUMP_FILE, "w") == 0)
					{

					}
				}
				else
				{
					if (errno == ENOENT)
					{
						if (fopen_s(&out, DUMP_FILE, "a") == 0)
						{

						}
					}
				}
				
				if (out)
				{

					fprintf(out, "Pkt: Port %04d len=%04d ", port, len);
					for (index = 0; index < len; index++)
					{
						fprintf(out, "%02x ", buf[index]);
					}
					fclose(out);
					DBGOUT(this, "%s", pktdump);
					free(pktdump);
				}
			}
			break;
	}
	return 0;
}

void init(void)
{
	ADDCL(cl);
	ADDDBGCL(dl);
}

void fini(void)
{
	DELCL(cl);
	DELDBGCL(dl);
}


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

