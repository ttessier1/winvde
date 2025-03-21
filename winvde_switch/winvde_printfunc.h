#pragma once

#include <varargs.h>

size_t asprintf(char ** buffer, char * fmt, ...);
size_t vasprintf(char** buffer, char* fmt, ...);