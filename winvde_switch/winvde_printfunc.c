#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include "winvde_printfunc.h"

void asprintf(char** buffer, char* fmt, ...)
{
	va_list list;
	int length = 0;
	if (!buffer)
	{
		errno = EINVAL;
		return;
	}
	va_start(list, fmt);
	length = vsnprintf(NULL, 0, fmt, list);
	va_end(list);
	*buffer = malloc(length+1);
	if (*buffer)
	{
		va_start(list, fmt);
		vsnprintf(*buffer, length + 1, fmt, list);
		va_end(list);
	}
	else
	{
		errno = ENOMEM;
		return;
	}
}

void avsprintf(char** buffer, char* fmt, va_list list)
{
	int length = 0;
	if (!buffer)
	{
		errno = EINVAL;
		return;
	}
	length = vsnprintf(NULL, 0, fmt, list);
	*buffer = malloc(length + 1);
	if (*buffer)
	{
		vsnprintf(*buffer, length + 1, fmt, list);
	}
	else
	{
		errno = ENOMEM;
		return ;
	}
}