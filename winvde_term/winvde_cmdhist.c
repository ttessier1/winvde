#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "winvde_cmdhist.h"
#include "winvde_termkeys.h"
#include "winvde_term.h"
#include "winvde_confuncs.h"

uint16_t command_history_count = 0;
struct command_history* cmd_history = NULL;
struct command_history* currentHistoryPtr = NULL;

int addCommandHistory(struct command_history** cmd_history, char* command, size_t length)
{
    struct command_history* ptrCommandHistory = NULL;
    struct command_history* saveCommandHistory = NULL;
    if (!cmd_history || !command || length <= 0)
    {
        errno = EINVAL;
        return -1;
    }
    uint32_t hash = calculateHash(command, length);
    ptrCommandHistory = *cmd_history;
    if (ptrCommandHistory)
    {
        saveCommandHistory = ptrCommandHistory;
        while (ptrCommandHistory)
        {
            if (ptrCommandHistory->checksum == hash)
            {
                return 0;
            }
            saveCommandHistory = ptrCommandHistory;
            ptrCommandHistory = ptrCommandHistory->next;
        }
        ptrCommandHistory = saveCommandHistory;
        ptrCommandHistory->next = (struct command_history*)malloc(sizeof(struct command_history));
        if (!ptrCommandHistory->next)
        {
            errno = ENOMEM;
            return -1;
        }
        memset(ptrCommandHistory->next, 0, sizeof(struct command_history));
        ptrCommandHistory->next->prev = ptrCommandHistory;
        ptrCommandHistory = ptrCommandHistory->next;

    }
    else
    {
        ptrCommandHistory = (struct command_history*)malloc(sizeof(struct command_history));
        if (!ptrCommandHistory)
        {
            errno = ENOMEM;
            return -1;
        }
        memset(ptrCommandHistory, 0, sizeof(struct command_history));
    }
    ptrCommandHistory->command = (char*)malloc(length + 1);
    if (!ptrCommandHistory->command)
    {
        errno = ENOMEM;
        return -1;
    }
    memcpy(ptrCommandHistory->command, command, length);
    ptrCommandHistory->command[length] = '\0';
    ptrCommandHistory->length = length;
    ptrCommandHistory->checksum = hash;
    ptrCommandHistory->lastUsed = time(NULL);
    command_history_count++;
    if (command_history_count > MAX_HISTORY)
    {
        deleteOldestHistory(cmd_history);
    }
    if (*cmd_history == NULL)
    {
        *cmd_history = ptrCommandHistory;
    }
    return 0;
}

int deleteOldestHistory(struct command_history** cmd_history)
{
    struct command_history* ptrCommandHistory = NULL;
    struct command_history* oldestHistory = NULL;
    time_t oldest = time(NULL);
    if (cmd_history && *cmd_history)
    {
        ptrCommandHistory = *cmd_history;
        while (ptrCommandHistory)
        {
            if (ptrCommandHistory->lastUsed < oldest)
            {
                oldestHistory = ptrCommandHistory;
                oldest = ptrCommandHistory->lastUsed;
            }
            ptrCommandHistory = ptrCommandHistory->next;
        }
        if (oldestHistory)
        {
            ptrCommandHistory = oldestHistory;
            if (ptrCommandHistory->prev)
            {
                ptrCommandHistory->prev->next = ptrCommandHistory->next;
            }
            if (ptrCommandHistory->next)
            {
                ptrCommandHistory->next->prev = ptrCommandHistory->prev;
            }
            if (ptrCommandHistory->command)
            {
                free(ptrCommandHistory->command);
                ptrCommandHistory->command = NULL;
            }
            if (*cmd_history == ptrCommandHistory)
            {
                *cmd_history = ptrCommandHistory->next;
            }
            free(ptrCommandHistory);
        }
        return 0;
    }
    errno = EINVAL;
    return -1;
}

uint32_t calculateHash(char* command, size_t length)
{
    uint64_t value = 0;
    uint8_t index = 0;
    char* ptrCommand = NULL;
    if (command != NULL)
    {
        ptrCommand = command;
        while (index < length)
        {
            index++;
            value += index * (*ptrCommand);
            ptrCommand++;
        }
        return value & 0xFFFF;
    }
    return 0;
}

void cleanHistory(struct command_history** cmd_history)
{
    struct command_history* ptrCommandHistory = NULL;
    struct command_history* saveCommandHistory = NULL;
    if (cmd_history && *cmd_history)
    {
        ptrCommandHistory = *cmd_history;
        while (ptrCommandHistory)
        {
            saveCommandHistory = ptrCommandHistory->next;
            if (ptrCommandHistory->command)
            {
                free(ptrCommandHistory->command);
                ptrCommandHistory->command = NULL;
            }
            free(ptrCommandHistory);
            ptrCommandHistory = saveCommandHistory;
        }
    }
}

void CommandHistUp()
{
    if (currentHistoryPtr == NULL)
    {
        currentHistoryPtr = cmd_history;
        if (currentHistoryPtr == NULL)
        {
            return;
        }
    }
    else
    {
        currentHistoryPtr = currentHistoryPtr->next;
    }
    if (currentHistoryPtr && currentHistoryPtr->command)
    {
        memcpy(std_input_buffer, currentHistoryPtr->command, currentHistoryPtr->length);
        std_input_length = (uint16_t)currentHistoryPtr->length;
        std_input_pos = (uint16_t)currentHistoryPtr->length;
        RestoreCursorPos();
        SaveCursorPos();
        fprintf(stdout, "%.*s", (int)(80 - strlength(prompt)), "                                                                              ");
        RestoreCursorPos();
        fprintf(stdout, "%.*s", std_input_length, std_input_buffer);
    }
    else
    {
        RestoreCursorPos();
        SaveCursorPos();
        fprintf(stdout, "%.*s", (int)(80 - strlength(prompt)), "                                                                              ");
        RestoreCursorPos();
        std_input_length = 0;
        std_input_pos = 0;
    }
}

void CommandHistDown()
{
    if (currentHistoryPtr == NULL)
    {
        currentHistoryPtr = cmd_history;
        if (currentHistoryPtr == NULL)
        {
            return;
        }
        else
        {
            // go to the end of the list
            while (currentHistoryPtr->next)
            {
                currentHistoryPtr = currentHistoryPtr->next;
            }
        }
    }
    else
    {
        currentHistoryPtr = currentHistoryPtr->prev;
    }
    if (currentHistoryPtr && currentHistoryPtr->command)
    {
        memcpy(std_input_buffer, currentHistoryPtr->command, currentHistoryPtr->length);
        std_input_length = (uint16_t)currentHistoryPtr->length;
        std_input_pos = (uint16_t)currentHistoryPtr->length;
        RestoreCursorPos();
        SaveCursorPos();
        fprintf(stdout, "%.*s", (int)80 - strlength(prompt), "                                                                              ");
        RestoreCursorPos();
        fprintf(stdout, "%.*s", std_input_length, std_input_buffer);
    }
    else
    {
        RestoreCursorPos();
        SaveCursorPos();
        fprintf(stdout, "%.*s", (int)80 - strlength(prompt), "                                                                              ");
        RestoreCursorPos();
        std_input_length = 0;
        std_input_pos = 0;
    }
}