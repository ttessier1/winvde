#pragma once

#define COMMAND_HELP1 "%-7s\t%-10s\t%-20s\n"
#define COMMAND_HELP2 "%-7s\t%-40s\n"

#define COMMAND_EXIT "exit"
#define COMMAND_CLS "cls"
#define COMMAND_DIR "dir"
#define COMMAND_CAT "cat"
#define COMMAND_CWD "getcwd"
#define COMMAND_HELP "help"
#define COMMAND_ECHO "echo"

#define COMMAND_EXIT_LENGTH strlen(COMMAND_EXIT)
#define COMMAND_CLS_LENGTH strlen(COMMAND_CLS)
#define COMMAND_DIR_LENGTH strlen(COMMAND_DIR)
#define COMMAND_CAT_LENGTH strlen(COMMAND_CAT)
#define COMMAND_CWD_LENGTH strlen(COMMAND_CWD)
#define COMMAND_HELP_LENGTH strlen(COMMAND_HELP)
#define COMMAND_ECHO_LENGTH strlen(COMMAND_ECHO)

struct command_parameter
{
    char* argument;
    size_t length;
    struct command_parameter* next;
};

extern uint8_t DoLoop;

void CleanUpCommand(struct command_parameter** commands);
int HandleCommand(const struct command_parameter* command);
int ParseCommand(const char* line, size_t length, struct command_parameter** commands);
int CollapseArgumentsToString(const struct command_parameter* argument, char** value);