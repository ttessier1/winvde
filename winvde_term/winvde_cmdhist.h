#pragma once

#include <stdint.h>
#include <time.h>

#define MAX_HISTORY 10

struct command_history
{
    char* command;
    size_t length;
    uint32_t checksum;
    time_t lastUsed;
    struct command_history* prev;
    struct command_history* next;
};

extern uint16_t command_history_count;
extern struct command_history* cmd_history;
extern struct command_history* ptr;

int addCommandHistory(struct command_history** cmd_history, char* std_input_buffer, size_t std_input_length);
int deleteOldestHistory(struct command_history** cmd_history);
void cleanHistory(struct command_history** cmd_history);
uint32_t calculateHash(char* command, size_t length);
int CollapseArgumentsToString(const struct command_parameter* argument, char** value);

void CommandHistUp();
void CommandHistDown();
