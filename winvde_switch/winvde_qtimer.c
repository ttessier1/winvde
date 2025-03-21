#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>

#include "winvde_qtimer.h"

#define QT_ALLOC_STEP 4
#define SIGALARM SIGTERM

struct qt_timer
{
	int qt_identifier; // timer id
	time_t qt_period;
	time_t qt_nextcall;
	unsigned int qt_times;
	void (*qt_call)();
	void * qt_arg;
};

struct qt_timer** qt_head; // head of the timer list
struct qt_timer* qt_free;

int qt_size; // size of active array

time_t g_qtime; // global epoch

int active_timers;
int timer_count;// timer id count

int ss_alarm, ss_old;
UINT_PTR timerId = 0;

HANDLE ghMutex = INVALID_HANDLE_VALUE;

time_t qtime()
{
	return g_qtime;
}

int qtime_csenter()
{
	DWORD dwWaitResult = 0;
	if (ghMutex != INVALID_HANDLE_VALUE)
	{
		dwWaitResult = WaitForSingleObject(
			ghMutex,    // handle to mutex
			INFINITE);  // no time-out interval
		switch (dwWaitResult)
		{
		case WAIT_OBJECT_0:

			break;
		case WAIT_ABANDONED:
			fprintf(stderr, "Wait Abandoned: %d\n", GetLastError());
			return -1;
			break;
		}
	}
	else
	{
		fprintf(stderr, __FUNCTION__ ": Mutex Not Initialized\n");
		return -1;
	}
	return 0;
}

void qtime_csexit()
{
	if (ghMutex != INVALID_HANDLE_VALUE)
	{
		if (!ReleaseMutex(ghMutex))
		{
			fprintf(stderr, __FUNCTION__ "Failed to release the mutex: %d\n", GetLastError());
		}
	}
	else
	{
		fprintf(stderr, __FUNCTION__ ": Mutex Not Initialized\n");
	}
}

void sig_alarm(HWND unnamedParam1,UINT unnamedParam2,UINT_PTR unnamedParam3,DWORD unnamedParam4)
{
	int first_index = 0;
	int second_index = 0;
	qtime_csenter();
	g_qtime++;

	if (qt_head)
	{
		for (first_index = 0, second_index = 0; first_index < active_timers; first_index++)
		{
			if (qt_head[first_index]->qt_times == 0)
			{
				qt_head[first_index]->qt_arg = qt_free;
				qt_free = qt_head[first_index];
			}
			else
			{
				if (g_qtime >= qt_head[first_index]->qt_nextcall)
				{
					if (qt_head[first_index]->qt_call)
					{
						qt_head[first_index]->qt_call(qt_head[first_index]->qt_arg);
					}
					qt_head[first_index]->qt_nextcall += qt_head[first_index]->qt_period;
					if (qt_head[first_index]->qt_times > 0)
					{
						qt_head[first_index]->qt_times--;
					}
				}
				if (first_index - second_index)
				{
					qt_head[second_index] = qt_head[first_index];
				}
				second_index++;
			}
		}
		active_timers=second_index;
	}
	qtime_csexit();
}

int qtimer_init()
{
	ghMutex = CreateMutex(NULL, FALSE, NULL);
	if (!ghMutex)
	{
		fprintf(stderr, __FUNCTION__ ":Failed to create the mutex\n");
		errno = EINVAL;
		return -1;
	}
	timerId = SetTimer(NULL,0x1234,1000, (TIMERPROC)sig_alarm);
	if (timerId == 0)
	{
		CloseHandle(ghMutex);
		ghMutex = NULL;
		errno = EINVAL;
		return -1;
	}
	return 0;
}

unsigned int qtimer_add(time_t period, int times, void(*call)(), void * arg)
{
	int index = 0;
	if (period > 0 && call && times >= 0)
	{
		qtime_csenter();
		if (active_timers >= qt_size)
		{
			int newmaxqt = qt_size + QT_ALLOC_STEP;
			qt_head = (struct qt_timer**)realloc(qt_head,newmaxqt*sizeof(struct qt_timer *));
			if (qt_head == NULL)
			{
				errno = ENOMEM;
				return -1;
			}
			qt_size = newmaxqt;
		}
		index = active_timers++;
		if (qt_free == NULL)
		{
			qt_free = malloc(sizeof(struct qt_timer));
			if (qt_free == NULL)
			{
				errno = ENOMEM;
				return -1;
			}
			qt_free->qt_arg = NULL;
		}
		qt_head[index] = qt_free;
		qt_free = qt_free->qt_arg;
		qt_head[index]->qt_identifier = timer_count;
		qt_head[index]->qt_period = period;
		qt_head[index]->qt_nextcall = g_qtime + period;
		qt_head[index]->qt_call = call;
		qt_head[index]->qt_arg = arg;
		qt_head[index]->qt_times = (times == 0) ? -1 : times;
		qtime_csexit();
		return qt_head[index]->qt_identifier;
	}
	return -1;
}

void qtimer_del(unsigned int timer_id)
{
	uint32_t index = 0;
	if (active_timers <= 0)
	{
		errno = EINVAL;
		return;
	}
	for (index = 0; index < (uint32_t)active_timers; index++)
	{
		if (timer_id == qt_head[index]->qt_identifier)
		{
			qt_head[index]->qt_times = 0;
			break;
		}
	}
}

#if defined(TEST_QTIMER)
void fun(void* arg)
{
	printf("FUN: %x\n", arg);
}

void exit_timer(void* arg)
{
	printf("Exit: %x\n", arg);
	PostMessage(NULL, WM_QUIT, 0, 0);
}

void main()
{
	qtimer_init();
	qtimer_add(7, 0, fun, 0xFFFF1234);
	qtimer_add(3, 0, fun, 0xFFFF2222);
	qtimer_add(4, 2, fun, 0xFFFF5555L);
	qtimer_add(30, 1, exit_timer, 0xFFFFFFFF);
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

#endif
