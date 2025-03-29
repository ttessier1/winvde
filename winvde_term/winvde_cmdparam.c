#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "winvde_cmdparam.h"
#include "winvde_term.h"



int HandleCommand(const struct command_parameter* command)
{
    size_t par_length = 0;
    size_t command_length = 0;
    char* tmpBuff = NULL;
    char* collapsedString = NULL;
    char resolved[MAX_PATH];
    HANDLE findHandle = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAA findData;
    struct command_parameter* argument = NULL;
    if (command != NULL && command->argument != NULL)
    {
        par_length = strlen(command->argument);
        command_length = COMMAND_EXIT_LENGTH;
        if (memcmp(command->argument, COMMAND_EXIT, min(par_length, command_length)) == 0 && par_length == command_length)
        {
            DoLoop = 0;
            return -2;
        }
        command_length = COMMAND_CLS_LENGTH;
        if (memcmp(command->argument, COMMAND_CLS, min(par_length, command_length)) == 0)
        {
            system("cls");
            return 0;
        }
        command_length = COMMAND_DIR_LENGTH;
        if (memcmp(command->argument, COMMAND_DIR, min(par_length, command_length)) == 0 && par_length == command_length)
        {
            if (command->next && command->next->argument != NULL)
            {
                argument = command->next;
                // naively get the first command and make sure its relative
                termGetRealPath(argument->argument, resolved);

                fprintf(stdout, "Resolved Path: %s\n", resolved);
                size_t resolved_length = strlen(resolved);
                size_t startup_length = strlen(STARTUP_FOLDER);
                if (memcmp(resolved, STARTUP_FOLDER, min(resolved_length, startup_length)) && startup_length <= resolved_length)
                {

                    tmpBuff = (char*)malloc(strlen(argument->argument) + strlen("\\*.*") + 1);
                    if (!tmpBuff)
                    {
                        fprintf(stderr, "Failed to allocate memory\n");
                        return -1;
                    }
                    strncpy_s(tmpBuff, strlen(argument->argument) + strlen("\\*.*") + 1, argument->argument, strlen(argument->argument) + strlen("\\*.*"));
                    strncat_s(tmpBuff, strlen(argument->argument) + strlen("\\*.*") + 1, "\\*.*", strlen("\\*.*"));
                    findHandle = FindFirstFileA(tmpBuff, &findData);

                    if (findHandle != INVALID_HANDLE_VALUE)
                    {
                        do
                        {
                            fprintf(stdout, "%s\n", findData.cFileName);
                        } while (FindNextFileA(findHandle, &findData));
                        if (tmpBuff)
                        {
                            free(tmpBuff);
                            tmpBuff = NULL;
                        }
                    }
                    else
                    {
                        if (tmpBuff)
                        {
                            free(tmpBuff);
                            tmpBuff = NULL;
                        }
                        return -1;
                    }
                }
                else
                {
                    fprintf(stderr, "Can not list the contents of this directory\n");
                    return -1;
                }
            }
            else
            {
                findHandle = FindFirstFileA("*.*", &findData);
                if (findHandle != INVALID_HANDLE_VALUE)
                {
                    do
                    {
                        fprintf(stdout, "%s\n", findData.cFileName);
                    } while (FindNextFileA(findHandle, &findData));
                }
                else
                {
                    return -1;
                }
            }
            return 0;

        }
        command_length = COMMAND_CAT_LENGTH;
        if (memcmp(command->argument, COMMAND_CAT, min(par_length, command_length)) == 0 && par_length == command_length)
        {
            if (command->next && command->next->argument != NULL)
            {
                argument = command->next;
                termGetRealPath(argument->argument, resolved);

                fprintf(stdout, "Resolved Path: %s\n", resolved);
                size_t resolved_length = strlen(resolved);
                size_t startup_length = strlen(STARTUP_FOLDER);
                if (memcmp(resolved, STARTUP_FOLDER, min(resolved_length, startup_length)) && startup_length <= resolved_length)
                {
                    FILE* file = NULL;
                    CHAR theChar = 0;

                    if (fopen_s(&file, argument->argument, "r") == 0)
                    {
                        while (1)
                        {
                            theChar = fgetc(file);
                            if (feof(file))
                            {
                                break;
                            }
                            fputc(theChar, stdout);
                        }
                        fclose(file);
                    }
                    else
                    {
                        fprintf(stderr, "Failed to open the file\n");
                        return -1;
                    }
                }
                else
                {
                    fprintf(stderr, "Can not open this path\n");
                    return -1;
                }
            }
            else
            {
                fprintf(stderr, "command requires argument\b");
                return -1;
            }
            return 0;

        }
        command_length = COMMAND_CWD_LENGTH;
        if (memcmp(command->argument, COMMAND_CWD, min(par_length, command_length)) == 0 && par_length == command_length)
        {
            char buff[MAX_PATH];
            _getcwd(buff, sizeof(buff));
            fprintf(stdout, "%s\n", buff);
            return 0;
        }
        command_length = COMMAND_HELP_LENGTH;
        if (memcmp(command->argument, COMMAND_HELP, min(par_length, command_length)) == 0 && par_length == command_length)
        {
            if (command->next == NULL)
            {
                fprintf(stderr, COMMAND_HELP1, "Command", "Arg", "Description");
                fprintf(stderr, COMMAND_HELP1, "-------", "----------", "--------------------");
                fprintf(stderr, COMMAND_HELP1, COMMAND_EXIT, "", "exit the terminal");
                fprintf(stderr, COMMAND_HELP1, COMMAND_CLS, "", "clear the terminal");
                fprintf(stderr, COMMAND_HELP1, COMMAND_DIR, "[path]", "give a relative directory listing");
                fprintf(stderr, COMMAND_HELP1, COMMAND_CAT, "[path]", "output a file to screen");
                fprintf(stderr, COMMAND_HELP1, COMMAND_CWD, "", "display the current path");
                fprintf(stderr, COMMAND_HELP1, COMMAND_ECHO, "[string]", "display the string");
                fprintf(stderr, COMMAND_HELP1, COMMAND_HELP, "", "display help");
                return 0;
            }
            else
            {
                argument = command->next;
                if (argument->argument != NULL)
                {
                    if (strcmp(argument->argument, COMMAND_EXIT) == 0)
                    {
                        fprintf(stderr, COMMAND_HELP2, COMMAND_EXIT, "There are no parameters to the exit command");
                        fprintf(stderr, COMMAND_HELP2, "", "");
                    }
                    return 0;
                }
                return -1;
            }

        }
        command_length = COMMAND_ECHO_LENGTH;
        if (memcmp(command->argument, COMMAND_ECHO, min(par_length, command_length)) == 0 && par_length == command_length)
        {
            if (command->next == NULL)
            {
                fprintf(stdout, "\n");
            }
            else
            {
                argument = command->next;
                if (CollapseArgumentsToString(argument, &collapsedString) == 0 && collapsedString != NULL)
                {
                    fprintf(stdout, "%s\n", collapsedString);
                    free(collapsedString);
                    return 0;
                }
                else
                {
                    fprintf(stderr, "Failed to collapse the string:%d\n", errno);
                    return -1;
                }
            }


            return 0;
        }
        fprintf(stderr, "Invalid Command\n");
        return -1;
    }
    return -1;
}

int ParseCommand(const char* line, size_t length, struct command_parameter** commands)
{
    fprintf(stdout, "\n");

    enum ParseCommandState { LeadingWhitespace, DquoteArgument, SquoteArgument, Argument, EndOfLine };
    uint8_t state = LeadingWhitespace;
    struct command_parameter* ptr = NULL;
    uint8_t foundFirstNonSpace = 0;
    const char* linePtr = line;
    const char* startPtr = NULL;
    const char* endPtr = NULL;

    uint16_t argument_index = -1;
    size_t pos = 0;
    if (!line || !commands)
    {
        errno = EINVAL;
        return -1;
    }
    if (*commands != NULL)
    {
        CleanUpCommand(commands);
    }
    ptr = (struct command_parameter*)malloc(sizeof(struct command_parameter));
    if (!ptr)
    {
        errno = ENOMEM;
        return -1;
    }
    *commands = ptr;
    memset(ptr, 0, sizeof(struct command_parameter));
    while (pos < length)
    {
        if ((*linePtr == ' ' || *linePtr == '\n' || *linePtr == '\t') && state == LeadingWhitespace)
        {
            linePtr++;
            pos++;
        }
        else if ((*linePtr == ' ' || *linePtr == '\n' || *linePtr == '\t') && state == Argument)
        {
            // This is a argument separator
            endPtr = linePtr;
            //fprintf(stdout, "Argument[%d]:%.*s Length:%lld\n", argument_index, (int)(endPtr - startPtr), startPtr, endPtr - startPtr);
            ptr->argument = (char*)malloc((endPtr - startPtr) + 1);
            if (!ptr->argument)
            {
                errno = ENOMEM;
                return -1;
            }
            strncpy_s(ptr->argument, (endPtr - startPtr) + 1, startPtr, endPtr - startPtr);
            ptr->length = (endPtr - startPtr);
            ptr->next = (struct command_parameter*)malloc(sizeof(struct command_parameter));
            if (!ptr->next)
            {
                errno = ENOMEM;
                return -1;
            }
            ptr = ptr->next;
            memset(ptr, 0, sizeof(struct command_parameter));
            state = LeadingWhitespace;
        }
        else
        {
            if (*linePtr == '"' && state == LeadingWhitespace)
            {
                state = DquoteArgument;
                // This is a 
            }
            else if (*linePtr == '"' && state == DquoteArgument)
            {
                // This is potentially the end of our argument
                state = Argument;
            }
            else if (*linePtr == '\'' && state == LeadingWhitespace)
            {
                state = SquoteArgument;
            }
            else if (*linePtr == '\'' && state == SquoteArgument)
            {
                state = Argument;
            }
            else if (state == LeadingWhitespace)
            {
                startPtr = linePtr;
                state = Argument;
                argument_index++;
            }
        }
        linePtr++;
        pos++;
    }
    if (argument_index > -1 && startPtr != NULL)
    {
        endPtr = linePtr;
        //fprintf(stdout, "Argument[%d]:%.*s Length:%lld\n", argument_index, (int)(endPtr - startPtr), startPtr, endPtr - startPtr);
        ptr->argument = (char*)malloc((endPtr - startPtr) + 1);
        if (!ptr->argument)
        {
            errno = ENOMEM;
            return -1;
        }
        strncpy_s(ptr->argument, (endPtr - startPtr) + 1, startPtr, endPtr - startPtr);
        ptr->length = (endPtr - startPtr);
    }

    return 0;
}

void CleanUpCommand(struct command_parameter** commands)
{
    struct command_parameter* cmdPtr = NULL;
    struct command_parameter* savePtr = NULL;
    if (commands && *commands)
    {
        cmdPtr = *commands;
        while (cmdPtr)
        {
            savePtr = cmdPtr->next;
            if (cmdPtr->argument)
            {
                free(cmdPtr->argument);
                cmdPtr->argument = NULL;
            }
            free(cmdPtr);
            cmdPtr = NULL;
            cmdPtr = savePtr;
        }
        *commands = NULL;
    }
}

int SetMainPath()
{
    char path[MAX_PATH];
    if (ExpandEnvironmentStringsA(STARTUP_FOLDER, path, MAX_PATH))
    {
        fprintf(stderr, "Attempting to set the path to: %s\n", path);
        if (_chdir(path))
        {
            switch (errno)
            {
            case ENOENT:
                fprintf(stderr, "Unable to locate the directory: %s\n", path);
                break;
            case EINVAL:
                fprintf(stderr, "Invalid buffer.\n");
                break;
            default:
                fprintf(stderr, "Unknown error.\n");
            }
            return -1;
        }
        return 0;

    }
    else
    {
        fprintf(stderr, "Failed to expand path:%s\n", path);
    }

    return -1;
}

int CollapseArgumentsToString(const struct command_parameter* argument, char** value)
{
    const struct command_parameter* ptr = NULL;
    size_t length = 0;
    size_t copy_length = 0;
    char* valuePtr = NULL;
    if (!argument || !value)
    {
        errno = EINVAL;
        return -1;
    }
    ptr = argument;
    while (ptr)
    {
        if (ptr->argument)
        {
            length += strlen(ptr->argument);
            if (ptr->next)
            {
                // account for space character
                length++;
            }
        }
        else
        {
            length++; // add a space on a null argument
        }
        ptr = ptr->next;
    }
    *value = malloc(length + 1);
    if ((*value) == NULL)
    {
        errno = ENOMEM;
        return -1;
    }
    valuePtr = *value;
    ptr = argument;
    while (ptr && copy_length <= length)
    {
        if (ptr->argument)
        {
            strncpy_s(valuePtr, ptr->length + 1, ptr->argument, ptr->length);
            valuePtr += ptr->length;
            copy_length += ptr->length;
            if (ptr->next)
            {
                // account for space character
                *valuePtr = ' ';
                valuePtr++;
                copy_length++;
            }
            else
            {
                *valuePtr = '\0';
            }
        }
        else
        {
            *valuePtr = ' ';
            valuePtr++;
            copy_length++;
        }
        ptr = ptr->next;
    }
    return 0;
}