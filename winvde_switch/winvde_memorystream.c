#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdio.h>

#include <memory.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "winvde_mgmt.h"

#define MEMORY_BUFFER_SIZE 1024

typedef struct _buffer_chain
{
    uint32_t buffer_id;
    struct _buffer_chain* prev;
    struct _buffer_chain* next;
    char buffer[MEMORY_BUFFER_SIZE];
} BUFFERCHAIN, * LPBUFFERCHAIN;

typedef struct _memory_file {
    int fd;             // File descriptor
    size_t buffer_pos;  // Current position in the buffer
    size_t buffer_end;  // End of valid data in the buffer
    size_t size;
    uint32_t buffer_index;
    int mode;           // Mode (read/write)
    struct _buffer_chain* buffer_chain;
    char* final_buffer;
    char** internal_buffer_ptr;
} MEMORYFILE, * LPMEMORYFILE;

struct _memory_file* open_memorystream(char** buffer, size_t** size);
size_t write_memorystream(struct _memory_file* file, const char* buff, size_t buffsize);
char* get_buffer(struct _memory_file* file);

struct _memory_file* open_memorystream(char** buffer, size_t** size)
{
    struct _memory_file* memorybuffer = NULL;
    if (!buffer || !size)
    {
        switch_errno = EINVAL;
        return NULL;
    }
    memorybuffer = (struct _memory_file*)malloc(sizeof(struct _memory_file));
    if (memorybuffer)
    {
        memset(memorybuffer, 0, sizeof(struct _memory_file));
        memorybuffer->size = 0;
        *size = &memorybuffer->size;
        memorybuffer->internal_buffer_ptr = buffer;

        memorybuffer->buffer_chain = (struct _buffer_chain*)malloc(sizeof(struct _buffer_chain));
        if (memorybuffer->buffer_chain)
        {
            memset(memorybuffer->buffer_chain, 0, sizeof(struct _buffer_chain));
            memorybuffer->buffer_index++;
            return memorybuffer;
        }
        else
        {
            free(memorybuffer);
            memorybuffer = NULL;
            switch_errno = ENOMEM;
            return NULL;
        }
    }
    return NULL;
}

size_t write_memorystream(struct _memory_file* file, const char* buff, size_t buffsize)
{
    size_t remaining = 0;
    size_t written = 0;
    size_t writtensave = 0;
    size_t buffoffset = 0;
    if (!file || !buff || buffsize == 0)
    {
        switch_errno = EINVAL;
        return -1;
    }
    if (file->buffer_pos >= MEMORY_BUFFER_SIZE)
    {
        switch_errno = EINVAL;
        return -1;
    }
    remaining = MEMORY_BUFFER_SIZE - file->buffer_pos;
    memcpy(&file->buffer_chain->buffer[file->buffer_pos], buff, min(remaining, buffsize));

    written += min(remaining, buffsize);
    buffoffset += written;
    file->buffer_pos += written;
    file->size += written;
    remaining = remaining - written;
    buffsize = buffsize - written;
    while (buffsize > 0)
    {
        writtensave += written;
        written = 0;
        file->buffer_pos = 0;
        file->buffer_chain->next = (struct _buffer_chain*)malloc(sizeof(struct _buffer_chain));
        if (file->buffer_chain->next)
        {
            // Items to update on a buffer replacement - 
            // this should not necessarily be recursive
            memset(file->buffer_chain->next, 0, sizeof(struct _buffer_chain));
            file->buffer_chain->next->prev = file->buffer_chain;
            file->buffer_chain = file->buffer_chain->next;
            file->buffer_chain->buffer_id = file->buffer_index++;
            remaining = MEMORY_BUFFER_SIZE;
            memcpy(&file->buffer_chain->buffer[file->buffer_pos], &buff[buffoffset], min(remaining, buffsize));
            written = min(remaining, buffsize);
            buffoffset += written;
            file->buffer_pos += written;
            file->size += written;
            remaining = remaining - written;
            buffsize = buffsize - written;
        }
        else
        {
            switch_errno = ENOMEM;
            return -1;
        }
    }
    writtensave += written;

    return writtensave;
}

char* get_buffer(struct _memory_file* file)
{
    struct _buffer_chain* ptr = NULL;
    size_t pos = 0;
    if (!file)
    {
        switch_errno = EINVAL;
        return NULL;
    }
    file->final_buffer = (char*)malloc(file->size + 1);
    
    if (file->final_buffer)
    {
        memset(file->final_buffer, 0, file->size + 1);
        *file->internal_buffer_ptr = file->final_buffer;
        ptr = file->buffer_chain;
        while (ptr->prev)
        {
            ptr = ptr->prev;
        }
        while (ptr)
        {

            if (ptr->next)
            {
                memcpy(&file->final_buffer[pos], ptr->buffer, MEMORY_BUFFER_SIZE);
                pos += MEMORY_BUFFER_SIZE;
            }
            else
            {
                memcpy(&file->final_buffer[pos], ptr->buffer, file->buffer_pos);
                pos += file->buffer_pos;
            }

            ptr = ptr->next;
        }
        return file->final_buffer;
    }
    return NULL;
}

int close_memorystream(struct _memory_file* file)
{
    struct _buffer_chain* ptr = NULL;
    struct _buffer_chain* save = NULL;
    int returnCode = -1;
    if (file != NULL)
    {
        ptr = file->buffer_chain;
        ptr = file->buffer_chain;
        while (ptr->prev)
        {
            ptr = ptr->prev;
        }
        while (ptr)
        {
            save = ptr->next;
            free(ptr);
            ptr = save;
        }
        if (file->final_buffer != NULL)
        {
            free(file->final_buffer);
        }
        free(file);
        returnCode = 0;
    }
    return returnCode;
}