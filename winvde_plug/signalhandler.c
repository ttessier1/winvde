
#include "signalhandler.h"
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>

void (*cleanupcopy)(void);

void sig_handler(int sig)
{
	cleanupcopy();
	signal(sig, SIG_DFL);
	if (sig == SIGTERM)
	{
		exit(0);
	}
}

void setsighandlers(void(*cleanup)(void))
{
	struct { 
		int sig;
		const char* name;
		int ignore;
	}
	signals[] =
	{
#if defined SIGHUP
		{SIGHUP,"SIGHUP",0},
#endif
		{SIGINT, "SIGINT",0},
#if defined SIGPIPE		
		{SIGPIPE,"SIGPIPE",1},
#endif
#if defined SIGALARM 
		{SIGALARM,"SIGALRM",1},
#endif
		{SIGTERM,"SIGTERM",0},
#if defined SIGUSR1
		{SIGUSR1,"SIGUSR1",1},
#endif
#if defined SIGUSR2
		{SIGUSR2,"SIGUSR2",1},
#endif
#if defined SIGPROF
		{SIGPROF,"SIGPROF",1},
#endif
#if defined SIGVTALRM
		{SIGVTALRM,"SIGVTALRM",1},
#endif

#if defined SIGPOLL
		{SIGPOLL,"SIGPOLL",1},
#endif
#if defined SIGSTKFLT
		{SIGSKFLT, "SIGSKFLT",1},
#endif
#if defined SIGIO
		{SIGIO,"SIGIO",1},
#endif
#if defined SIGPWR
		{SIGPWR,"SIGPWR",1},
#endif
#if defined SIGUNUSED
		{SIGUNUSED,"SIGUNUSED",1},
#endif
		{0,NULL,0},
	};

	int index = 0;

	for (index = 0; signals[index].sig != 0; index++)
	{
		if (signals[index].ignore==0)
		{
			if (signal(signals[index].sig, sig_handler) )
			{
				perror("Setting Handler\n");
			}
		}
	}
	cleanupcopy = cleanup;
}