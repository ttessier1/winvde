#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include "winvde_printfunc.h"

size_t asprintf(char** buffer, char* fmt, ...)
{
	va_list list;
	size_t length = 0;
	if (!buffer)
	{
		errno = EINVAL;
		return 0;
	}
	va_start(list, fmt);
	length = vsnprintf(NULL, 0, fmt, list);
	va_end(list);
	*buffer = malloc(length+1);
	if (*buffer)
	{
		va_start(list, fmt);
		length = vsnprintf(*buffer, length + 1, fmt, list);
		va_end(list);
		return length;
	}
	else
	{
		errno = ENOMEM;
		return 0;
	}
}

size_t vasprintf(char** buffer, char* fmt, va_list list)
{
	size_t length = 0;
	if (!buffer)
	{
		errno = EINVAL;
		return 0;
	}
	length = vsnprintf(NULL, 0, fmt, list);
	*buffer = malloc(length + 1);
	if (*buffer)
	{
		return vsnprintf(*buffer, length + 1, fmt, list);
	}
	else
	{
		errno = ENOMEM;
		return 0;
	}
}