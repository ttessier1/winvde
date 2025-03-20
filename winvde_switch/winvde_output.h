#pragma once

#include <stdio.h>

void printlog(int priority, const char* format, ...);
void printoutc(FILE* f, const char* format, ...);
