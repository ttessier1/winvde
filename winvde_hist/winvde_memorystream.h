#pragma once
#include <stdio.h>

//typedef struct _memory_file * MFILE;

struct _memory_file* open_memorystream(char** buffer, size_t** size);
size_t write_memorystream(struct _memory_file* file, const char* buff, size_t buffsize);
size_t writechar_memorystream(struct _memory_file* file, const char thechar);
char* get_buffer(struct _memory_file* file);
int close_memorystream(struct _memory_file* file);