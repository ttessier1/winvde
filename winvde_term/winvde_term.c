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

#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"winvde_hist.lib")

char* prompt;

void cleanup(void);
void sig_handler(int sig);
char* copy_header_prompt(SOCKET vdefd, int termfd, const char* sock);
void setsighandlers();

#define BUFFER_SIZE 1024

int std_input_pos;
char std_input_buffer[BUFFER_SIZE];

char errorbuff[BUFFER_SIZE];

int main(const int argc, const char** argv)
{
	WSADATA wsaData;

	struct sockaddr_un sun;
	SOCKET server_socket = INVALID_SOCKET;
	DWORD one = 1;

	fd_set read_fds;
	fd_set write_fds;
	fd_set exception_fds;

	int sel = 0;

	int rv;
	int buffer_ready = 0;
	struct vdehiststat* vdehst=NULL;

	FD_ZERO(&read_fds);
	FD_ZERO(&write_fds);
	FD_ZERO(&exception_fds);

	if(WSAStartup(MAKEWORD(2, 2), &wsaData) == SOCKET_ERROR)
	{
		fprintf(stderr, "Failed to startup sockets:%d\n", WSAGetLastError());
		exit(0);
	}
	setsighandlers();
	//tcgetattr(STD_INPUT_HANDLE, &tiop);
	atexit(cleanup);
	sun.sun_family = PF_UNIX;
	snprintf(sun.sun_path, sizeof(sun.sun_path), "%s", argv[1]);
	//asprintf(&prompt,"vdterm[%s]: ",argv[1]);
	if ((server_socket = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
		perror("Socket opening error");
		WSACleanup();
		exit(-1);
	}
	if ((rv = connect(server_socket, (struct sockaddr*)(&sun), sizeof(sun))) < 0) {
		strerror_s(errorbuff, sizeof(errorbuff), errno);
		fprintf(stderr,"Socket connecting error: %s %d\n", errorbuff, WSAGetLastError());
		WSACleanup();
		exit(-1);
	}
	if (ioctlsocket(server_socket, FIONBIO, &one) < 0)
	{
		perror("Failed to set non blocking mode on socket\n");
		//exit(-1);
	}
	
	prompt = copy_header_prompt(server_socket, STD_OUTPUT_HANDLE, (char*)argv[1]);
	if (prompt == NULL)
	{
		fprintf(stderr, "Failed to read the prompt\n");
		goto CleanUp;
	}
	vdehst = vdehist_new(STD_INPUT_HANDLE, server_socket);
	fprintf(stdout, "%*.s\n",(int)strlen(prompt)+1,prompt);
	while (1) {
		FD_SET(server_socket,&read_fds);
		FD_SET(server_socket, &write_fds);
		FD_SET(server_socket, &exception_fds);
		sel = select(1, &read_fds, &write_fds, &exception_fds, NULL);
		if (sel < 0)
		{
			continue;
		}
		if (sel == 0)
		{
			if (_kbhit())
			{
				std_input_buffer[std_input_pos] = _getche();
				if (std_input_buffer[std_input_pos] == '\n' || std_input_buffer[std_input_pos] == '\r')
				{
					buffer_ready = 1;
				}
				else if (std_input_pos >= BUFFER_SIZE - 1)
				{
					buffer_ready = 1;
				}
				else
				{
					std_input_pos++;
				}
			}
		}
		else if (sel > 0)
		{
			if (FD_ISSET(server_socket, &read_fds))
			{
				vdehist_mgmt_to_term(vdehst);
			}
		}
		if (buffer_ready == 1)
		{
			if (vdehist_term_to_mgmt(vdehst,std_input_buffer,std_input_pos) != 0)
			{
				goto CleanUp;
			}
		}
	}
CleanUp:
	if (server_socket != INVALID_SOCKET)
	{
		closesocket(server_socket);
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
	fd_set read_fds;
	fd_set write_fds;
	fd_set exception_fds;

	FD_ZERO(&read_fds);
	FD_ZERO(&write_fds);
	FD_ZERO(&exception_fds);

	while (1)
	{
		FD_SET(vdefd, &read_fds);
		FD_SET(vdefd, &write_fds);
		FD_SET(vdefd, &exception_fds);

		sel = select(3, &read_fds, &write_fds, &exception_fds, NULL);
		if (sel < 0)
		{
			continue;
		}
		if (FD_ISSET(vdefd, &read_fds))
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
					fprintf(stdout, "%*.s\n", n + 1, buf);
					//_write(termfd, buf, n + 1);
					asprintf(&prompt, "%s[%s]: ", buf + n + 1, sock);
					return prompt;
				}
				else
				{
					fprintf(stdout, "%*.s\n", n, buf);
					//_write(termfd, buf, n);
				}
			}
		}
		if (FD_ISSET(vdefd, &write_fds))
		{
			fprintf(stdout, "Write FD Available\n");
		}
		if (FD_ISSET(vdefd, &exception_fds))
		{
			fprintf(stdout, "Write FD Available\n");
		}
	}
	return NULL;
}