#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#define FDSET_SIZE 1 // potential optimization
#include <WinSock2.h>
#include <afunix.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <io.h>
#include <conio.h>

#include "winvde_hist.h"
#include "winvde_memorystream.h"
#include "winvde_printfunc.h"
#include "winvde_termkeys.h"
#include "winvde_confuncs.h"
#include "winvde_cmdhist.h"

#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"winvde_hist.lib")

char* prompt;

uint8_t DoLoop = 1;

void cleanup(void);
void sig_handler(int sig);
char* copy_header_prompt(SOCKET vdefd, int termfd, const char* sock);
void setsighandlers();

int SetupKeyHandlers();

#define BUFFER_SIZE 1024

char errorbuff[BUFFER_SIZE];

int main(const int argc, const char** argv)
{
	WSADATA wsaData;

	struct sockaddr_un sun;
	SOCKET serverSocket = INVALID_SOCKET;
	DWORD one = 1;

	LPWSAPOLLFD wsaPollFD = NULL;

	int sel = 0;

	int rv;
	//int buffer_ready = 0;
	struct vdehiststat* vdehst=NULL;

	if(WSAStartup(MAKEWORD(2, 2), &wsaData) == SOCKET_ERROR)
	{
		fprintf(stderr, "Failed to startup sockets:%d\n", WSAGetLastError());
		exit(0);
	}

	SetupKeyHandlers();

	setsighandlers();
	//tcgetattr(STD_INPUT_HANDLE, &tiop);
	atexit(cleanup);
	sun.sun_family = PF_UNIX;
	snprintf(sun.sun_path, sizeof(sun.sun_path), "%s", argv[1]);
	//asprintf(&prompt,"vdterm[%s]: ",argv[1]);
	if ((serverSocket = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
		perror("Socket opening error");
		WSACleanup();
		exit(-1);
	}
	if ((rv = connect(serverSocket, (struct sockaddr*)(&sun), sizeof(sun))) < 0) {
		strerror_s(errorbuff, sizeof(errorbuff), errno);
		fprintf(stderr, "Socket connecting error: %s %d\n", errorbuff, WSAGetLastError());
		WSACleanup();
		exit(-1);
	}
	if (ioctlsocket(serverSocket, FIONBIO, &one) < 0)
	{
		perror("Failed to set non blocking mode on socket\n");
		//exit(-1);
	}
	wsaPollFD = (LPWSAPOLLFD)malloc(sizeof(WSAPOLLFD) * 2);
	if (wsaPollFD == NULL)
	{
		fprintf(stderr, "Failed to allocate memory for WSAPoll Descriptor\n");
		goto CleanUp;
	}
	wsaPollFD[0].fd = serverSocket;
	wsaPollFD[0].events = POLLIN;
	wsaPollFD[0].revents = 0;
	wsaPollFD[1].fd = serverSocket;
	wsaPollFD[1].events = POLLOUT;
	wsaPollFD[1].revents = 0;
	prompt = copy_header_prompt(serverSocket, STD_OUTPUT_HANDLE, (char*)argv[1]);
	if (prompt == NULL)
	{
		fprintf(stderr, "Failed to read the prompt\n");
		goto CleanUp;
	}
	vdehst = vdehist_new(_fileno(stdout), serverSocket);
	SaveCursorPos();
	fprintf(stdout, "\033[32m%.*s\033[0m", (int)strlen(prompt) + 1, prompt);
	while (DoLoop) {

		sel = WSAPoll(wsaPollFD, 2, 0);
		if (sel < 0)
		{
			fprintf(stderr, "Failed to poll:%d\n", WSAGetLastError());
			break;
		}
		if (DoKbHit() != 0)
		{

			fprintf(stderr, "Failed to check key press\n");
			break;
		}
		if (sel > 0)
		{
			if (wsaPollFD[0].revents & POLLIN)
			{
				if (vdehist_mgmt_to_term(vdehst) < 0)
				{
					goto CleanUp;
				}
				SaveCursorPos();
			}
			if (wsaPollFD[1].revents & POLLOUT && bufferReady == 1 && std_input_pos>0)
			{

				if (vdehist_term_to_mgmt(vdehst, std_input_buffer, std_input_pos) != 0)
				{
					goto CleanUp;
				}
				
				fprintf(stdout, "\033[32m%.*s\033[0m", (int)strlen(prompt) + 1, prompt);
				SaveCursorPos();
				std_input_buffer[std_input_length + 1] = '\0';
				addCommandHistory(&cmd_history, std_input_buffer, std_input_length);
				std_input_pos = 0;
				std_input_length = 0;
				bufferReady = 0;
			}
			if (bufferReady == 1 && std_input_pos == 0)
			{
				fprintf(stdout, "\n\033[32m%.*s\033[0m", (int)strlen(prompt) + 1, prompt);
				SaveCursorPos();
				std_input_buffer[std_input_length + 1] = '\0';
				addCommandHistory(&cmd_history, std_input_buffer, std_input_length);
				std_input_pos = 0;
				std_input_length = 0;
				bufferReady = 0;

			}
			if ((wsaPollFD[0].revents & POLLHUP) ||
				(wsaPollFD[1].revents & POLLHUP) )
			{
				fprintf(stderr, "ServerClosed Connection\n");
				goto CleanUp;
			}
		}
	}
CleanUp:
	if (wsaPollFD != NULL)
	{
		free(wsaPollFD);
		wsaPollFD = NULL;
	}
	if (serverSocket != INVALID_SOCKET)
	{
		closesocket(serverSocket);
	}
	WSACleanup();
    return 0;
}


void cleanup(void)
{
	fprintf(stderr, "\n");
	//tcsetattr(STDIN_FILENO, TCSAFLUSH, &tiop);
}

void sig_handler(int sig)
{
	cleanup();
	signal(sig, SIG_DFL);
	if (sig == SIGTERM)
	{
		_exit(0);
	}
	else
	{
		TerminateProcess(GetCurrentProcess(), sig);
	}
}

static void setsighandlers()
{
	/* setting signal handlers.
	 * sets clean termination for SIGHUP, SIGINT and SIGTERM, and simply
	 * ignores all the others signals which could cause termination. */
	struct { int sig; const char* name; int ignore; } signals[] = {
#if defined(SIGHUP)
		{ SIGHUP, "SIGHUP", 0 },
#endif
#if defined(SIGINT)
		{ SIGINT, "SIGINT", 0 },
#endif
#if defined(SIGPIPE)
		{ SIGPIPE, "SIGPIPE", 1 },
#endif
#if defined(SIGALRM)
		{ SIGALRM, "SIGALRM", 1 },
#endif
#if defined(SIGTERM)
		{ SIGTERM, "SIGTERM", 0 },
#endif
#if defined(SIGUSR1)
		{ SIGUSR1, "SIGUSR1", 1 },
#endif
#if defined(SIGUSR2)
		{ SIGUSR2, "SIGUSR2", 1 },
#endif
#if defined(SIGPROF)
		{ SIGPROF, "SIGPROF", 1 },
#endif
#if defined(SIGVTALRM)
		{ SIGVTALRM, "SIGVTALRM", 1 },
#endif
#ifdef VDE_LINUX
		{ SIGPOLL, "SIGPOLL", 1 },
#ifdef SIGSTKFLT
		{ SIGSTKFLT, "SIGSTKFLT", 1 },
#endif
#if defined(SIGIO)
		{ SIGIO, "SIGIO", 1 },
#endif
#if defined(SIGPWR)
		{ SIGPWR, "SIGPWR", 1 },
#endif
#ifdef SIGUNUSED
		{ SIGUNUSED, "SIGUNUSED", 1 },
#endif
#endif
#ifdef VDE_DARWIN
		{ SIGXCPU, "SIGXCPU", 1 },
		{ SIGXFSZ, "SIGXFSZ", 1 },
#endif
		{ 0, NULL, 0 }
	};

	int i;
	_crt_signal_t sig;
	for (i = 0; signals[i].sig != 0; i++)
	{
		sig = signal(signals[i].sig, signals[i].ignore ? SIG_IGN : sig_handler);
		if ( sig == SIG_ERR)
		{
			strerror_s(errorbuff, sizeof(errorbuff), errno);
			fprintf(stderr, "Error setting handler for %s: %s\n", signals[i].name,errorbuff);
		}
	}
}


static char* copy_header_prompt(SOCKET vdefd, int termfd, const char* sock)
{
	char buf[BUFSIZE];
	int n;
	int sel = 0;
	char* prompt;

	LPWSAPOLLFD wsaPollFD = NULL;
	
	wsaPollFD = (LPWSAPOLLFD)malloc(sizeof(WSAPOLLFD)*2);
	if (!wsaPollFD)
	{
		fprintf(stderr, "Failed to allocate memory for WSAPoll Descriptors\n");
		return NULL;
	}

	wsaPollFD[0].fd = vdefd;
	wsaPollFD[0].events = POLLIN;
	wsaPollFD[0].revents = 0;
	wsaPollFD[1].fd = vdefd;
	wsaPollFD[1].events = POLLOUT;
	wsaPollFD[1].revents = 0;

	while (1)
	{
		sel = WSAPoll(wsaPollFD,2,0);
		if (sel < 0)
		{
			fprintf(stderr, "Failed to Poll: %d\n",WSAGetLastError());
			break;
		}
		if (wsaPollFD[0].revents&POLLIN)
		{
			while ((n = recv(vdefd, buf, BUFSIZE, 0)) > 0)
			{
				if (buf[n - 2] == '$' &&
					buf[n - 1] == ' ')
				{
					n -= 2;
					buf[n] = 0;
					while (n > 0 && buf[n] != '\n')
					{
						n--;
					}
					fprintf(stdout, "%.*s\n", n + 1, buf);
					//_write(termfd, buf, n + 1);
					asprintf(&prompt, "%s[%s]: ", buf + n + 1, sock);
					return prompt;
				}
				else
				{
					fprintf(stdout, "%.*s\n", n, buf);
					//_write(termfd, buf, n);
				}
			}
		}
	}
	if (wsaPollFD!=NULL)
	{
		free(wsaPollFD);
		wsaPollFD = NULL;
	}
	return NULL;
}

int SetupKeyHandlers()
{
	AttachKeyHandler(NUMPAD_0_KEY,ToggleInsert);

	AttachKeyHandler(NUMPAD_LEFT_KEY, MoveCursorPositionLeft);
	AttachKeyHandler(NUMPAD_RIGHT_KEY, MoveCursorPositionRight);

	AttachKeyHandler(NUMPAD_UP_KEY, CommandHistUp);
	AttachKeyHandler(NUMPAD_DOWN_KEY, CommandHistDown);

	AttachKeyHandler(NUMPAD_PGUP_KEY, ScrollUp);
	AttachKeyHandler(NUMPAD_PGDOWN_KEY, ScrollDown);
	
	AttachKeyHandler(KEYBOARD_INSERT, ToggleInsert);
	
	AttachKeyHandler(KEYBOARD_LEFT, MoveCursorPositionLeft);
	AttachKeyHandler(KEYBOARD_RIGHT, MoveCursorPositionRight);
	
	AttachKeyHandler(KEYBOARD_UP, CommandHistUp);
	AttachKeyHandler(KEYBOARD_DOWN, CommandHistDown);

	AttachKeyHandler(KEYBOARD_PGUP, ScrollUp);
	AttachKeyHandler(KEYBOARD_PGDWN, ScrollDown);

	return 0;
}