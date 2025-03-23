#pragma once

#include <stdarg.h>

size_t asprintf(char ** buffer, const char * fmt, ...);
size_t vasprintf(char** buffer, const char* fmt, va_list list);