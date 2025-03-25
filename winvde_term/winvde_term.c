#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>
#include <afunix.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <io.h>

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

#define BUFSIZE 1024

char errorbuff[BUFSIZE];

int main(const int argc, const char** argv)
{
	WSADATA wsaData;

	struct sockaddr_un sun;
	SOCKET fd = INVALID_SOCKET;
	DWORD one = 1;
	int rv;
	//struct termios newtiop;
	static struct pollfd pfd[] = {
		{STD_INPUT_HANDLE,POLLIN | POLLHUP,0},
		{STD_INPUT_HANDLE,POLLIN | POLLHUP,0} };
	//static int fileout[]={STDOUT_FILENO,STDOUT_FILENO};
	struct vdehiststat* vdehst;

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
	if ((fd = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
		perror("Socket opening error");
		WSACleanup();
		exit(-1);
	}
	if ((rv = connect(fd, (struct sockaddr*)(&sun), sizeof(sun))) < 0) {
		strerror_s(errorbuff, sizeof(errorbuff), errno);
		fprintf(stderr,"Socket connecting error: %s %d\n", errorbuff, WSAGetLastError());
		WSACleanup();
		exit(-1);
	}
	if (ioctlsocket(fd, FIONBIO, &one) < 0)
	{
		perror("Failed to set non blocking mode on socket\n");
		//exit(-1);
	}
	
	/*newtiop = tiop;
	newtiop.c_cc[VMIN] = 1;
	newtiop.c_cc[VTIME] = 0;
	newtiop.c_lflag &= ~ICANON;
	newtiop.c_lflag &= ~ECHO;
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &newtiop);
	flags = fcntl(fd, F_GETFL);
	flags |= O_NONBLOCK;
	fcntl(fd, F_SETFL, flags);
	pfd[1].fd = fd;*/
	pfd[1].fd = fd;
	prompt = copy_header_prompt(fd, STD_INPUT_HANDLE, (char*)argv[1]);
	vdehst = vdehist_new(STD_INPUT_HANDLE, fd);
	_write(STD_INPUT_HANDLE, prompt, (int)strlen(prompt) + 1);
	while (1) {
		WSAPoll(pfd, 2, -1);
		//printf("POLL %d %d\n",pfd[0].revents,pfd[1].revents);
		if (pfd[0].revents & POLLHUP ||
			pfd[1].revents & POLLHUP)
			exit(0);
		if (pfd[0].revents & POLLIN) {
			if (vdehist_term_to_mgmt(vdehst) != 0)
			{
				WSACleanup();
				exit(0);
			}
		}
		if (pfd[1].revents & POLLIN)
		{
			vdehist_mgmt_to_term(vdehst);
		}
		//printf("POLL RETURN!\n");
	}

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
		if ( sig == NULL)
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
	char* prompt;
	while (1)
	{
		struct pollfd wfd = { vdefd,POLLIN | POLLHUP,0 };
		WSAPoll(&wfd, 1, -1);
		while ((n = recv(vdefd, buf, BUFSIZE,0)) > 0)
		{
			if (buf[n - 2] == '$' &&
				buf[n - 1] == ' ')
			{
				n -= 2;
				buf[n] = 0;
				while (n > 0 && buf[n] != '\n')
					n--;
				_write(termfd, buf, n + 1);
				asprintf(&prompt, "%s[%s]: ", buf + n + 1, sock);
				return prompt;
			}
			else
			{
				_write(termfd, buf, n);
			}
		}
	}
}