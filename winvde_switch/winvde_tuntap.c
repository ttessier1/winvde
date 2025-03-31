#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winsock2.h>
#include <wintun.h>
#include <winmeta.h>
#include <ws2ipdef.h>
#include <iphlpapi.h>
#include <mstcpip.h>
#include <ip2string.h>
#include <stdlib.h>
#include <string.h>
#include <winternl.h>

#include "winvde_loglevel.h"
#include "winvde_module.h"
#include "winvde_output.h"
#include "winvde_port.h"
#include "winvde_getopt.h"
#include "winvde_descriptor.h"
#include "winvde_modsupport.h"
#include "winvde_type.h"
#include "winvde_mgmt.h"

static WINTUN_CREATE_ADAPTER_FUNC* WintunCreateAdapter;
static WINTUN_CLOSE_ADAPTER_FUNC* WintunCloseAdapter;
static WINTUN_OPEN_ADAPTER_FUNC* WintunOpenAdapter;
static WINTUN_GET_ADAPTER_LUID_FUNC* WintunGetAdapterLUID;
static WINTUN_GET_RUNNING_DRIVER_VERSION_FUNC* WintunGetRunningDriverVersion;
static WINTUN_DELETE_DRIVER_FUNC* WintunDeleteDriver;
static WINTUN_SET_LOGGER_FUNC* WintunSetLogger;
static WINTUN_START_SESSION_FUNC* WintunStartSession;
static WINTUN_END_SESSION_FUNC* WintunEndSession;
static WINTUN_GET_READ_WAIT_EVENT_FUNC* WintunGetReadWaitEvent;
static WINTUN_RECEIVE_PACKET_FUNC* WintunReceivePacket;
static WINTUN_RELEASE_RECEIVE_PACKET_FUNC* WintunReleaseReceivePacket;
static WINTUN_ALLOCATE_SEND_PACKET_FUNC* WintunAllocateSendPacket;
static WINTUN_SEND_PACKET_FUNC* WintunSendPacket;

#define MAXCMD 128
#define MODULENAME "tuntap"

HMODULE hWinTunTap = INVALID_HANDLE_VALUE;

struct winvde_module module;
struct mod_support module_functions;

unsigned int tap_type;

HANDLE QuitEvent;
BOOL HaveQuit;
DWORD LastError = 0;

typedef struct inittuntap {
	char* tap_dev;
	struct inittuntap* next;
} INITTUNTAP, *LPINITUNTAP;

struct inittuntap* hinit_tap  = NULL ;

struct option long_options[] = {
	{"tap", 1, 0, 't'},
};

#define Nlong_options (sizeof(long_options)/sizeof(struct option));

#define X(Name) ((*(FARPROC *)&Name = GetProcAddress(Wintun, #Name)) == NULL)

struct endpoint* newtap(char* dev);
struct inittuntap* add_init_tap(struct inittuntap* p, const char* arg);
struct inittuntap* free_init_tap(struct inittuntap* p);

HMODULE
InitializeWintun(void)
{
	HMODULE Wintun = LoadLibrary(L"wintun.dll");
	if (!Wintun)
	{
		return NULL;
	}
	if (X(WintunCreateAdapter) || 
		X(WintunCloseAdapter) || 
		X(WintunOpenAdapter) || 
		X(WintunGetAdapterLUID) ||
		X(WintunGetRunningDriverVersion) || 
		X(WintunDeleteDriver) || 
		X(WintunSetLogger) || 
		X(WintunStartSession) ||
		X(WintunEndSession) || 
		X(WintunGetReadWaitEvent) || 
		X(WintunReceivePacket) || 
		X(WintunReleaseReceivePacket) ||
		X(WintunAllocateSendPacket) || 
		X(WintunSendPacket))
	{
		DWORD LastError = GetLastError();
		FreeLibrary(Wintun);
		SetLastError(LastError);
		return NULL;
	}
	return Wintun;
}

void CALLBACK ConsoleLogger(_In_ WINTUN_LOGGER_LEVEL Level, _In_ DWORD64 Timestamp, _In_z_ const WCHAR* LogLine)
{
	SYSTEMTIME SystemTime;
	FileTimeToSystemTime((FILETIME*)&Timestamp, &SystemTime);
	WCHAR LevelMarker;
	switch (Level)
	{
	case WINTUN_LOG_INFO:
		LevelMarker = L'+';
		break;
	case WINTUN_LOG_WARN:
		LevelMarker = L'-';
		break;
	case WINTUN_LOG_ERR:
		LevelMarker = L'!';
		break;
	default:
		return;
	}
	fwprintf(
		stderr,
		L"%04u-%02u-%02u %02u:%02u:%02u.%04u [%c] %s\n",
		SystemTime.wYear,
		SystemTime.wMonth,
		SystemTime.wDay,
		SystemTime.wHour,
		SystemTime.wMinute,
		SystemTime.wSecond,
		SystemTime.wMilliseconds,
		LevelMarker,
		LogLine);
}

static DWORD64 Now(VOID)
{
	LARGE_INTEGER Timestamp;
	NtQuerySystemTime(&Timestamp);
	return Timestamp.QuadPart;
}

DWORD
DoLogError(_In_z_ const WCHAR* Prefix, _In_ DWORD Error)
{
	WCHAR* SystemMessage = NULL, * FormattedMessage = NULL;
	FormatMessageW(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_MAX_WIDTH_MASK,
		NULL,
		HRESULT_FROM_SETUPAPI(Error),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(void*)&SystemMessage,
		0,
		NULL);
	FormatMessageW(
		FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_ARGUMENT_ARRAY |
		FORMAT_MESSAGE_MAX_WIDTH_MASK,
		SystemMessage ? L"%1: %3(Code 0x%2!08X!)" : L"%1: Code 0x%2!08X!",
		0,
		0,
		(void*)&FormattedMessage,
		0,
		(va_list*)(DWORD_PTR[]) { (DWORD_PTR)Prefix, (DWORD_PTR)Error, (DWORD_PTR)SystemMessage });
	if (FormattedMessage)
		ConsoleLogger(WINTUN_LOG_ERR, Now(), FormattedMessage);
	LocalFree(FormattedMessage);
	LocalFree(SystemMessage);
	return Error;
}

DWORD
LogLastError(_In_z_ const WCHAR* Prefix)
{
	DWORD LastError = GetLastError();
	DoLogError(Prefix, LastError);
	SetLastError(LastError);
	return LastError;
}

void Log(_In_ WINTUN_LOGGER_LEVEL Level, _In_z_ const WCHAR* Format, ...)
{
	WCHAR LogLine[0x200];
	va_list args;
	va_start(args, Format);
	_vsnwprintf_s(LogLine, _countof(LogLine), _TRUNCATE, Format, args);
	va_end(args);
	ConsoleLogger(Level, Now(), LogLine);
}



BOOL WINAPI
CtrlHandler(_In_ DWORD CtrlType)
{
	switch (CtrlType)
	{
	case CTRL_C_EVENT:
	case CTRL_BREAK_EVENT:
	case CTRL_CLOSE_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
		Log(WINTUN_LOG_INFO, L"Cleaning up and shutting down...");
		HaveQuit = TRUE;
		SetEvent(QuitEvent);
		return TRUE;
	}
	return FALSE;
}

int send_tap(SOCKET fd_ctl, SOCKET fd_data, void* packet, int len, uint16_t port)
{
	int n;

	n = len - send(fd_ctl, packet, len,0);
	if (n > len) {
		int rv = switch_errno;
		if (rv != EAGAIN && rv != EWOULDBLOCK)
		{
			strerror_s(errorbuff, sizeof(errorbuff), switch_errno);
			printlog(LOG_WARNING, "send_tap port %d: %s", port, errorbuff);
		}
		else
		{
			rv = EWOULDBLOCK;
		}
		return -rv;
	}
	return n;
}

void usage(void)
{
	fprintf(stdout,
		"(opts from tuntap module)\n"
		"  -t, --tap TAP              Enable routing through TAP tap interface\n"
	);
}

int parseopt(const int c, const char* optarg)
{
	int outc = 0;
	switch (c) {
	case 't':
		hinit_tap = add_init_tap(hinit_tap, optarg);
		break;
	default:
		outc = c;
	}
	return outc;
}

int init()
{
	struct inittuntap* p = NULL;
	if (hinit_tap != NULL)
	{
		hWinTunTap = InitializeWintun();
		if (hWinTunTap != NULL)
		{
			WintunSetLogger(ConsoleLogger);
			Log(WINTUN_LOG_INFO, L"Wintun library loaded");

			tap_type = add_type(&module, 1);
			for (p = hinit_tap; p != NULL; p = p->next)
			{
				if (newtap(p->tap_dev) == NULL)
				{
					printlog(LOG_ERR, "ERROR OPENING tap interface: %s", p->tap_dev);
				}
			}
			hinit_tap = free_init_tap(hinit_tap);
		}
	}
	return 0;
}

void handle_io(unsigned char type, SOCKET fd, int revents, void* private_data)
{
	int len = 0;
	struct endpoint* ep = private_data;
	struct bipacket packet;
#ifdef VDE_PQ2
	if (revents & POLLOUT)
	{
		handle_out_packet(ep);
	}
#endif
	if (revents & POLLIN)
	{
		len = send(fd, (char*) & (packet.p), sizeof(struct packet), 0);
		if (len < 0)
		{
			if (switch_errno != EAGAIN && switch_errno != EWOULDBLOCK)
			{
				strerror_s(errorbuff, sizeof(errorbuff), switch_errno);
				printlog(LOG_WARNING, "Reading tap data: %s", errorbuff);
			}
		}
		else if (len == 0)
		{
			if (switch_errno != EAGAIN && switch_errno != EWOULDBLOCK)
			{
				strerror_s(errorbuff,sizeof(errorbuff),switch_errno);
				printlog(LOG_WARNING, "EOF tap data port: %s", errorbuff);
			}
			/* close tap! */
		}
		else if (len >= ETH_HEADER_SIZE)
		{
			handle_in_packet(ep, &(packet.p), len);
		}
	}
}

struct inittuntap* add_init_tap(struct inittuntap* p, const char* arg)
{
	if (p == NULL)
	{
		p = malloc(sizeof(struct inittuntap));
		if (p == NULL)
		{
			strerror_s(errorbuff, sizeof(errorbuff), switch_errno);
			printlog(LOG_WARNING, "Malloc Tap init:%s\n", errorbuff);
		}
		else
		{
			p->tap_dev = _strdup(optarg);
			p->next = NULL;
		}
	}
	else
	{
		p->next = add_init_tap(p->next, arg);
	}
	return(p);
}

struct inittuntap* free_init_tap(struct inittuntap* p)
{
	if (p != NULL)
	{
		free_init_tap(p->next);
		free(p);
	}
	return NULL;
}

struct endpoint* newtap(char* dev)
{
	SOCKET tap_fd = INVALID_SOCKET;
	struct endpoint* ep;
	WINTUN_SESSION_HANDLE Session;

	GUID ExampleGuid = { 0xdeadbabe, 0xcafe, 0xbeef, { 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef } };
	WINTUN_ADAPTER_HANDLE Adapter = WintunCreateAdapter(L"WinVde", L"SwitchTunnel", &ExampleGuid);
	if (!Adapter)
	{
		LastError = GetLastError();
		DoLogError(L"Failed to create adapter", LastError);
		goto cleanupQuit;
	}

	DWORD Version = WintunGetRunningDriverVersion();
	Log(WINTUN_LOG_INFO, L"Wintun v%u.%u loaded", (Version >> 16) & 0xff, (Version >> 0) & 0xff);

	MIB_UNICASTIPADDRESS_ROW AddressRow;
	InitializeUnicastIpAddressEntry(&AddressRow);
	WintunGetAdapterLUID(Adapter, &AddressRow.InterfaceLuid);
	AddressRow.Address.Ipv4.sin_family = AF_INET;
	AddressRow.Address.Ipv4.sin_addr.S_un.S_addr = htonl((10 << 24) | (6 << 16) | (7 << 8) | (7 << 0)); /* 10.6.7.7 */
	AddressRow.OnLinkPrefixLength = 24; /* This is a /24 network */
	AddressRow.DadState = IpDadStatePreferred;
	LastError = CreateUnicastIpAddressEntry(&AddressRow);
	if (LastError != ERROR_SUCCESS && LastError != ERROR_OBJECT_ALREADY_EXISTS)
	{
		DoLogError(L"Failed to set IP address", LastError);
		goto cleanupAdapter;
	}

	Session = WintunStartSession(Adapter, 0x400000);
	if (!Session)
	{
		LastError = LogLastError(L"Failed to create adapter");
		goto cleanupAdapter;
	}

	Log(WINTUN_LOG_INFO, L"Launching threads and mangling packets...");
	/*tap_fd = open_tap(dev);
	if (tap_fd > 0)
	{*/
		ep = setup_ep(0, tap_fd, tap_fd, -1, &module_functions);
		if (ep != NULL) {
			setup_description(ep, dev);
			add_fd(tap_fd, tap_type, module.module_tag, ep);
		}
		return ep;
	/* }
	else
		return NULL;
		*/
	cleanupAdapter:
		;
	cleanupQuit:
		;
		return NULL;
}

void cleanup(unsigned char type, SOCKET fd, void* private_data)
{
	if (fd >= 0)
	{
		closesocket(fd);
	}
	if(hWinTunTap != NULL)
	{
		FreeLibrary(hWinTunTap);
	}
}

#if defined WIN32
int open_tap(char* dev)
{
	int rv = 0;

	/*struct ifreq ifr;
	int fd;

	if ((fd = open("/dev/tun", O_RDWR)) < 0) {
		printlog(LOG_ERR, "Failed to open /dev/tun %s", strerror(switch_errno));
		return(-1);
	}
	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
	strncpy(ifr.ifr_name, dev, sizeof(ifr.ifr_name) - 1);
	/*printf("dev=\"%s\", ifr.ifr_name=\"%s\"\n", ifr.ifr_name, dev);*/
	/*if (ioctl(fd, TUNSETIFF, (void*)&ifr) < 0) {
		printlog(LOG_ERR, "TUNSETIFF failed %s", strerror(switch_errno));
		close(fd);
		return(-1);
	}
	return(fd);*/
	return rv;
}
#endif

static void delep(SOCKET fd_ctl, SOCKET fd_data, void* descr)
{
	if (fd_ctl != INVALID_SOCKET)
	{
		remove_fd(fd_ctl);
	}
	if (descr)
	{
		free(descr);
	}
}

void start_tuntap(void)
{
	module.module_name = MODULENAME;
	module.options = Nlong_options;
	module.module_options = long_options;
	module.usage = usage;
	module.parse_options = parseopt;
	module.init = init;
	module.handle_io = handle_io;
	module.cleanup = cleanup;
	
	module_functions.modname = MODULENAME;
	module_functions.sender = send_tap;
	module_functions.delep = delep;

	add_module(&module);
}
