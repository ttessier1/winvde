#include <winmeta.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "winvde_switch.h"
#include "winvde_loglevel.h"

void printlog(int priority, const char* format, ...)
{
	va_list arg;

	va_start(arg, format);

	//if (logok)
		//vsyslog(priority, format, arg);
	//else {
		fprintf(stderr, "%s: ", prog);
		vfprintf(stderr, format, arg);
		fprintf(stderr, "\n");
	//}
	va_end(arg);
}

void printoutc(FILE* f, const char* format, ...)
{
	va_list arg;

	va_start(arg, format);
	if (f) {
		vfprintf(f, format, arg);
		fprintf(f, "\n");
	}
	else
		printlog(LOG_INFO, format, arg);
	va_end(arg);
}
