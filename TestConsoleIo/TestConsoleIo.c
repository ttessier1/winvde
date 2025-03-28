// TestConsoleIo.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <fileapi.h >
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <stdint.h>
#include <conio.h>
#include <direct.h>
#include <sys/stat.h>
#include <io.h>
#include <time.h>

#define STARTUP_FOLDER  "%USERPROFILE%\\.winvde2\\"

#define STD_INPUT_BUFF_SIZE 1024

#define MAXSYMLINKS 10


#define EXIST_OK 0
#define X_OK 01
#define W_OK 02
#define WX_OK 03
#define R_OK 04
#define RX_OK 05
#define RW_OK 06
#define RWX_OK 07

#define WV_S_IFMT   0x1F000 // File type mask
#define WV_S_IFLNK  0x10000 // is link
#define WV_S_IFDIR  0x04000 // Directory
#define WV_S_IFCHR  0x02000 // Character special
#define WV_S_IFIFO  0x01000 // Pipe
#define WV_S_IFREG  0x08000 // Regular
#define WV_S_IREAD  0x00100 // Read permission, owner
#define WV_S_IWRITE 0x00080 // Write permission, owner
#define WV_S_IEXEC  0x00040 // Execute/search permission, owner

struct winvde_stat {
    _dev_t         st_dev;
    _ino_t         st_ino;
    uint32_t       st_mode;
    short          st_nlink;
    short          st_uid;
    short          st_gid;
    _dev_t         st_rdev;
    __int64        st_size;
    __time64_t     st_atime;
    __time64_t     st_mtime;
    __time64_t     st_ctime;
};

struct command_parameter
{
    char* argument;
    struct command_parameter* next;
};

struct command_history
{
    char* command;
    size_t length;
    uint32_t checksum;
    time_t lastUsed;
    struct command_history* prev;
    struct command_history* next;
};

uint16_t std_input_pos = 0;
uint16_t std_input_length = 0;
char std_input_buffer[STD_INPUT_BUFF_SIZE];
char std_input_cpy_buffer[STD_INPUT_BUFF_SIZE];

char  prompt[] = "winvde$:";

#define COMMAND_HELP1 "%-7s\t%-10s\t%-20s\n"
#define COMMAND_HELP2 "%-7s\t%-40s\n"

#define COMMAND_EXIT "exit"
#define COMMAND_CLS "cls"
#define COMMAND_DIR "dir"
#define COMMAND_CAT "cat"
#define COMMAND_CWD "getcwd"
#define COMMAND_HELP "help"

#define COMMAND_EXIT_LENGTH strlen(COMMAND_EXIT)
#define COMMAND_CLS_LENGTH strlen(COMMAND_CLS)
#define COMMAND_DIR_LENGTH strlen(COMMAND_DIR)
#define COMMAND_CAT_LENGTH strlen(COMMAND_CAT)
#define COMMAND_CWD_LENGTH strlen(COMMAND_CWD)
#define COMMAND_HELP_LENGTH strlen(COMMAND_HELP)


uint8_t insertMode = 0;
uint8_t DoLoop = 1;
uint8_t bufferReady = 0;
uint16_t command_history_count = 0;

void SaveCursorPos();
void RestoreCursorPos();
void ToggleInsert();
void MoveCursorPositionLeft();
void MoveCursorPositionRight();
void SetScrollRegion();
void ScrollUp();
void ScrollDown();
void InsertLine();
void CleanUpCommand(struct command_parameter** commands);
int HandleCommand(const struct command_parameter* command);
int ParseCommand(const char* line, size_t length, struct command_parameter** commands);
int SetMainPath();
char* termGetRealPath(const char* name, char* resolved);
size_t termReadLink(const char* path, char* buffer, size_t bufsize);
int termStat(const char* filename, struct winvde_stat* stat);
int addCommandHistory(struct command_history** cmd_history,char * std_input_buffer, size_t std_input_length);
int deleteOldestHistory(struct command_history** cmd_history);
void cleanHistory(struct command_history** cmd_history);
uint32_t calculateHash(char* command, size_t length);

int main(const int argc, const char ** argv)
{
    char input_char = 0;
    int commandResult = 0;
    struct command_parameter* command=NULL;
    struct command_history * cmd_history= NULL;
    struct command_history* ptr = NULL;
    

    if (SetMainPath() == -1)
    {
        fprintf(stderr, "Failed to set the main path\n");
        return -1;
    }
    
    SetScrollRegion();
    fprintf(stdout, "\033[32m%s\033[0m", prompt);
    SaveCursorPos();
    while (DoLoop)
    {
        if (_kbhit())
        {
            input_char = _getch();// dont output
            if (input_char == '\b') // backspace
            {
                if (std_input_pos > 0)
                {
                    _putch('\b');
                    _putch(' ');
                    _putch(input_char);
                    std_input_pos--;
                    std_input_length--;
                    continue;
                }
            }
            else if (input_char == 0)
            {
                input_char = _getch();
                switch (input_char)
                {
                case 'S': // numpad . with numlock off
                    fprintf(stderr,"[.]");
                    continue;
                    break;
                case 'R': // numpad 0
                    //fprintf(stderr, "[NmPd0]");
                    ToggleInsert();
                    continue;
                    break;
                case 'H': //numpad up
                    fprintf(stderr, "[NmPdUp]");
                    continue;
                    break;
                case 'P': //numpad down
                    fprintf(stderr, "[NmPdDwn]");
                    continue;
                    break;
                case 'K': //mumpar left
                    //fprintf(stderr, "[NmPdLeft]");
                    MoveCursorPositionLeft();
                    continue;
                    break;
                case 'M': // numpar right
                    //fprintf(stderr, "[NmPdRight]");
                    MoveCursorPositionRight();
                    continue;
                    break;
                case 'G':// numpoad home
                    fprintf(stderr, "[NmPdHome]");
                    continue;
                    break;
                case 'O'://numpad end
                    fprintf(stderr, "[NmPdEnd]");
                    continue;
                    break;
                case 'I': // numpad pgup
                   // fprintf(stderr, "[NmPdPgUp]");
                    ScrollUp();
                    continue;
                    break;
                case 'Q': // numpad pgdn
                    //fprintf(stderr, "[NmPdPgDwn]");
                    ScrollDown();
                    continue;
                    break;
                case 59: // 59 F1
                    fprintf(stderr, "[F1]");
                    continue;
                    break;
                case 60: // 60 F2
                    fprintf(stderr, "[F2]");
                    continue;
                    break;
                case 61: // 61 F3
                    fprintf(stderr, "[F3]");
                    continue;
                    break;
                case 62: // 62 F4
                    fprintf(stderr, "[F4]");
                    continue;
                    break;
                case 63: // 63 F5
                    fprintf(stderr, "[F5]");
                    continue;
                    break;
                case 64: // 64 F6
                    fprintf(stderr, "[F6]");
                    continue;
                    break;
                case 65: // 65 F7
                    fprintf(stderr, "[F7]");
                    continue;
                    break;
                case 66: // 66 F8
                    fprintf(stderr, "[F8]");
                    continue;
                    break;
                case 67: // 67 F9
                    fprintf(stderr, "[F9]");
                    continue;
                    break;
                case 68: // 68 F10
                    fprintf(stderr, "[F10]");
                    continue;
                    break;
                // case 5 key while numlock off does nothing
                default:
                    fprintf(stderr, "[Unknown -%d]",input_char);
                    continue;
                }
            }
            else if (input_char == -32)
            {
                input_char = _getch();
                switch (input_char)
                {
                case 'G'://home key
                    fprintf(stderr, "[Home]");
                    continue;
                    break;
                case 'O': //end keu
                    fprintf(stderr, "[End]");
                    continue;
                    break;
                case 'R'://in key
                    //fprintf(stderr, "[Ins]");
                    ToggleInsert();
                    continue;
                    break;
                case 'S': //del key
                    fprintf(stderr, "[Del]");
                    continue;
                    break;
                case 'I':// pgup
                    //fprintf(stderr, "[PgUp]");
                    ScrollUp();
                    continue;
                    break;
                case 'Q'://pgdown
                    //fprintf(stderr, "[PgDwn]");
                    ScrollDown();
                    continue;
                    break;
                case 'H'://up arrow
                    //fprintf(stderr, "[Up]");
                    if (ptr == NULL)
                    {
                        ptr = cmd_history;
                        if (ptr == NULL)
                        {
                            continue;
                        }
                    }
                    else
                    {
                        ptr = ptr->next;
                    }
                    if (ptr && ptr->command)
                    {
                        memcpy(std_input_buffer, ptr->command, ptr->length);
                        std_input_length = (uint16_t)ptr->length;
                        std_input_pos = (uint16_t)ptr->length;
                        RestoreCursorPos();
                        SaveCursorPos();
                        fprintf(stdout, "%.*s",(int) 80 - strlen(prompt), "                                                                              ");
                        RestoreCursorPos();
                        fprintf(stdout,"%.*s",std_input_length,std_input_buffer);
                    }
                    else
                    {
                        RestoreCursorPos();
                        SaveCursorPos();
                        fprintf(stdout, "%.*s", (int)80 - strlen(prompt), "                                                                              ");
                        RestoreCursorPos();
                        std_input_length = 0;
                        std_input_pos = 0;
                    }
                    continue;
                    break;
                case 'P': //down arrow
                    //fprintf(stderr, "[Dwn]");
                    if (ptr == NULL)
                    {
                        ptr = cmd_history;
                        if (ptr == NULL)
                        {
                            continue;
                        }
                        else
                        {
                            // go to the end of the list
                            while (ptr->next)
                            {
                                ptr = ptr->next;
                            }
                        }
                    }
                    else
                    {
                        ptr = ptr->prev;
                    }
                    if (ptr && ptr->command)
                    {
                        memcpy(std_input_buffer, ptr->command, ptr->length);
                        std_input_length = (uint16_t)ptr->length;
                        std_input_pos = (uint16_t)ptr->length;
                        RestoreCursorPos();
                        SaveCursorPos();
                        fprintf(stdout, "%.*s", (int)80 - strlen(prompt), "                                                                              ");
                        RestoreCursorPos();
                        fprintf(stdout,"%.*s", std_input_length, std_input_buffer);
                    }
                    else
                    {
                        RestoreCursorPos();
                        SaveCursorPos();
                        fprintf(stdout, "%.*s", (int)80 - strlen(prompt), "                                                                              ");
                        RestoreCursorPos();
                        std_input_length = 0;
                        std_input_pos = 0;
                    }
                    continue;
                    break;
                case 'K': // left arrow
                    //fprintf(stderr, "[Left]");
                    MoveCursorPositionLeft();
                    continue;
                    break;
                case 'M'://right arrow
                    //fprintf(stderr, "[Right]");
                    MoveCursorPositionRight();
                    continue;
                    break;
                case -122:// F12
                    fprintf(stderr, "[F12]");
                    continue;
                    break;
                default:
                    fprintf(stderr, "[Unknown -%d]", input_char);
                    continue;
                }
                //fprintf(stdout, "\033[32mSpecial Char:\033[30m %c %d %.*s\n", input_char, input_char, std_input_pos, std_input_buffer);
                std_input_buffer[std_input_pos] = input_char;
                std_input_pos++;
                if (std_input_pos >= STD_INPUT_BUFF_SIZE)
                {

                    std_input_pos = 0;
                    bufferReady = 1;

                }
            }
            else
            {
                if (input_char == 0x1b) // escape
                {
                    fprintf(stderr, "[Esc]");
                    continue;
                }
                if (std_input_length == std_input_pos)
                {
                    _putch(input_char);
                }
                else
                {
                    if (insertMode == 0 && input_char!='\r' && !input_char!='\n')
                    {
                        _putch(input_char);
                        fprintf(stdout, "\0337");
                        printf("%.*s", std_input_length-std_input_pos,& std_input_buffer[std_input_pos]);
                        fprintf(stdout, "\0338");
                        memcpy(std_input_cpy_buffer, &std_input_buffer[std_input_pos], std_input_length - std_input_pos);
                        std_input_buffer[std_input_pos] = input_char;
                        memcpy(&std_input_buffer[std_input_pos + 1],std_input_cpy_buffer, min(STD_INPUT_BUFF_SIZE-std_input_pos, std_input_length - std_input_pos));
                        std_input_pos++;
                        std_input_length++;
                        continue;
                    }
                }
                
                if (input_char == '\n'|| input_char == '\r')
                {
                    bufferReady = 1;
                    if (std_input_length > 0)
                    {
                        if (ParseCommand(std_input_buffer, std_input_length, &command) == -1)
                        {
                            fprintf(stderr, "Failed to parse the command\n");
                            break;
                        }
                        if (HandleCommand(command) == 0)
                        {
                            fprintf(stdout, "\033[32m%s\033[0m", prompt);
                            SaveCursorPos();
                            addCommandHistory(&cmd_history, std_input_buffer, std_input_length);
                            std_input_pos = 0;
                            std_input_length = 0;
                            bufferReady = 0;
                            
                            


                            continue;
                        }
                    }
                }
                else
                {
                    std_input_buffer[std_input_pos] = input_char;
                    std_input_pos++;
                    std_input_length++;
                    if (std_input_pos >= STD_INPUT_BUFF_SIZE)
                    {
                       
                        std_input_pos = 0;
                        std_input_length = 0;
                        bufferReady = 1;
                    }
                }
            }
        }
        if (bufferReady)
        {
            
            /*if (std_input_pos > 0)
            {
                fprintf(stdout, "\n%.*s\n", std_input_length, std_input_buffer);
            }
            else
            {
                fprintf(stdout, "\n");
            }*/
            fprintf(stdout, "\033[32m%s\033[0m", prompt);
            std_input_pos = 0;
            std_input_length = 0;
            bufferReady = 0;
        }
    }
    if (cmd_history)
    {
        cleanHistory(&cmd_history);
    }
    
    if (command!=NULL)
    {
        CleanUpCommand(&command);
    }
    return 0;
}

void MoveCursorPositionLeft()
{
    if (std_input_pos > 0)
    {
        fprintf(stdout, "\033[1D");
        std_input_pos--;
    }
}

void MoveCursorPositionRight()
{
    if (std_input_pos < std_input_length)
    {
        fprintf(stdout,"\033[1C");
        std_input_pos++;
    }
}

void SaveCursorPos()
{
    fprintf(stdout, "\0337");
}

void RestoreCursorPos()
{
    fprintf(stdout, "\0338");
}

void ToggleInsert()
{
    if (insertMode == 0)
    {
        insertMode = 1;
        fprintf(stdout, "\033[1 q");
    }
    else
    {
        insertMode = 0;
        fprintf(stdout, "\033[5 q");
    }
}

void ScrollUp()
{
    fprintf(stdout, "\033[1S");
}

void ScrollDown()
{
    fprintf(stdout, "\033[1T");
}

void SetScrollRegion()
{
    fprintf(stdout, "\033[1;24");
}

void InsertLine()
{
    fprintf(stdout, "\033[1L");
}

int HandleCommand(const struct command_parameter* command)
{
    size_t par_length = 0;
    size_t command_length = 0;
    char* tmpBuff = NULL;
    char resolved[MAX_PATH];
    HANDLE findHandle=INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAA findData;
    struct command_parameter* argument = NULL;
    if (command != NULL && command->argument != NULL)
    {
        par_length = strlen(command->argument);
        command_length = COMMAND_EXIT_LENGTH;
        if (memcmp(command->argument, COMMAND_EXIT, min(par_length, command_length)) == 0 && par_length ==command_length)
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
            _getcwd(buff,sizeof(buff));
            fprintf(stdout, "%s\n",buff);
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
                fprintf(stderr, COMMAND_HELP1, COMMAND_HELP, "", "display help");
                return 0;
            }
            else
            {
                argument = command->next;
                if (argument->argument != NULL)
                {
                    if (strcmp(argument->argument,COMMAND_EXIT)==0)
                    {
                        fprintf(stderr, COMMAND_HELP2,COMMAND_EXIT,"There are no parameters to the exit command");
                        fprintf(stderr, COMMAND_HELP2, "", "");
                    }
                    return 0;
                }
            }
            
        }
        fprintf(stderr, "Invalid Command\n");
        return -1;
    }
    return -1;
}

int ParseCommand(const char* line, size_t length, struct command_parameter ** commands)
{
    fprintf(stdout, "\n");
    
    enum ParseCommandState { LeadingWhitespace, DquoteArgument, SquoteArgument, Argument, EndOfLine };
    uint8_t state = LeadingWhitespace;
    struct command_parameter * ptr = NULL;
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
    while (pos<length)
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
            fprintf(stdout,"Argument[%d]:%.*s Length:%lld\n", argument_index, (int)(endPtr - startPtr), startPtr, endPtr - startPtr);
            ptr->argument = (char*)malloc((endPtr - startPtr)+1);
            if (!ptr->argument)
            {
                errno = ENOMEM;
                return -1;
            }
            strncpy_s(ptr->argument, (endPtr - startPtr)+1,startPtr, endPtr - startPtr);
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
    if (argument_index > -1 && startPtr !=NULL)
    {
        endPtr = linePtr;
        fprintf(stdout, "Argument[%d]:%.*s Length:%lld\n", argument_index,(int)(endPtr - startPtr), startPtr, endPtr - startPtr);
        ptr->argument = (char*)malloc((endPtr - startPtr) + 1);
        if (!ptr->argument)
        {
            errno = ENOMEM;
            return -1;
        }
        strncpy_s(ptr->argument, (endPtr - startPtr)+1,startPtr, endPtr - startPtr);
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
                fprintf(stderr,"Unable to locate the directory: %s\n", path);
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


char* termGetRealPath(const char* name, char* resolved)
{
    char* dest = NULL;
    char* buf = NULL;
    char* extra_buf = NULL;
    const char* start = NULL;
    const char* end = NULL;
    const char* resolved_limit;
    char* resolved_ptr = NULL;
    char* resolved_root = NULL;
    char* ret_path = NULL;
    int num_links = 0;
    struct winvde_stat pst;
    size_t n = 0;
    if (!name || !resolved)
    {
        errno = EINVAL;
        goto abort;
    }
    resolved_ptr = resolved;
    resolved_root = resolved_ptr + 1;
    if (name[0] == '\0')
    {
        /* As per Single Unix Specification V2 we must return an error if
           the name argument points to an empty string.  */
        errno = ENOENT;
        goto abort;
    }

    if ((buf = (char*)malloc(MAX_PATH + 1)) == NULL) {
        errno = ENOMEM;
        goto abort;
    }

    if ((extra_buf = (char*)malloc(MAX_PATH + 1)) == NULL) {
        errno = ENOMEM;
        goto abort;
    }

    resolved_limit = resolved_ptr + MAX_PATH;

    /* relative path, the first char is not '\\' */
    if (!strstr(name, "C:\\") && !strstr(name, "\\"))
    {
        if (!_getcwd(resolved, MAX_PATH))
        {
            resolved[0] = '\0';
            goto abort;
        }

        dest = strchr(resolved, '\0');
    }
    else
    {
        /* absolute path */
        dest = resolved_root;
        resolved[0] = '\\';

        /* special case "/" */
        if (name[1] == 0)
        {
            *dest = '\0';
            ret_path = resolved;
            goto cleanup;
        }
    }

    /* now resolved is the current wd or "/", navigate through the path */
    for (start = end = name; *start; start = end)
    {
        n = 0;
        if (strstr(start + 3, ":\\"))
        {
            start += 6;
        }
        if (strstr(start + 1, ":\\"))
        {
            start += 3;
        }
        /* Skip sequence of multiple path-separators.  */
        while (*start == '\\')
            ++start;



        /* Find end of path component.  */
        for (end = start; *end && *end != '\\'; ++end);

        if (end - start == 0)
            break;
        else if (end - start == 1 && start[0] == '.')
            /* nothing */;
        else if (end - start == 2 && start[0] == '.' && start[1] == '.')
        {
            /* Back up to previous component, ignore if at root already.  */
            if (dest > resolved_root)
                while ((--dest)[-1] != '\\');
        }
        else
        {
            if (dest[-1] != '\\')
                *dest++ = '\\';

            if (dest + (end - start) >= resolved_limit)
            {
                errno = ENAMETOOLONG;
                if (dest > resolved_root)
                    dest--;
                *dest = '\0';
                goto abort;
            }

            /* copy the component, don't use mempcpy for better portability */
            dest = (char*)memcpy(dest, start, end - start) + (end - start);
            *dest = '\0';

            /*check the dir along the path */
            if (termStat(resolved, &pst) < 0)
                goto abort;
            else
            {
                /* this is a symbolic link, thus restart the navigation from
                 * the symlink location */
                if (pst.st_mode & WV_S_IFLNK)
                {
                    size_t len;

                    if (++num_links > MAXSYMLINKS)
                    {
                        errno = ELOOP;
                        goto abort;
                    }

                    /* symlink! */
                    n = termReadLink(resolved_ptr, buf, MAX_PATH);
                    resolved_ptr = buf;
                    if (n == 0xFFFFFFFFFFFFFFFF)
                        goto abort;

                    buf[n] = '\0';

                    len = strlen(end);
                    if ((long)(n + len) >= MAX_PATH)
                    {
                        errno = ENAMETOOLONG;
                        goto abort;
                    }

                    /* Careful here, end may be a pointer into extra_buf... */
                    memmove(&extra_buf[n], end, len + 1);
                    name = end = memcpy(extra_buf, buf, n);

                    if (strstr(&buf[0], "\\\\?\\C:\\"))
                    {
                        dest = resolved_root + 2;
                    }
                    else if (buf[0] == '\\')
                    {
                        dest = resolved_root;	/* It's an absolute symlink */
                    }
                    else if (strstr(&buf[1], ":\\"))
                    {
                        dest = resolved_root + 2;
                    }

                    else
                        /* Back up to previous component, ignore if at root already: */
                        if (dest > resolved + 1)
                            while ((--dest)[-1] != '\\');
                }
                else if (*end == '\\' && (pst.st_mode & S_IFDIR) == 0)
                {
                    errno = ENOTDIR;
                    goto abort;
                }
                else if (*end == '\\')
                {
                    if (_access(resolved, R_OK) != 0)
                    {
                        errno = EACCES;
                        goto abort;
                    }
                }
            }
        }
    }
    if (dest > resolved + 1 && dest[-1] == '/')
        --dest;
    *dest = '\0';

    ret_path = resolved_ptr;
    goto cleanup;

abort:
    ret_path = NULL;
cleanup:
    if (buf) free(buf);
    if (extra_buf) free(extra_buf);
    return ret_path;
}

int termStat(const char* filename, struct winvde_stat* stat)
{
    int rc = 0;
    DWORD dwAttributes = 0;
    struct _stat64 stat_real;
    if (!filename || !stat)
    {
        errno = EINVAL;
        return -1;
    }
    rc = _stat64(filename, &stat_real);
    if (rc == 0)
    {
        stat->st_atime = stat_real.st_atime;
        stat->st_ctime = stat_real.st_ctime;
        stat->st_mtime = stat_real.st_mtime;
        stat->st_dev = stat_real.st_dev;
        stat->st_gid = stat_real.st_gid;
        stat->st_ino = stat_real.st_ino;
        stat->st_mode = stat_real.st_mode;
        stat->st_nlink = stat_real.st_nlink;
        stat->st_rdev = stat_real.st_rdev;
        stat->st_size = stat_real.st_size;
        stat->st_uid = stat_real.st_uid;
        dwAttributes = GetFileAttributesA(filename);
        if (dwAttributes == INVALID_FILE_ATTRIBUTES)
        {
            rc = -1;
        }
        else if (dwAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
        {
            stat->st_mode = stat_real.st_mode | WV_S_IFLNK;
        }
    }
    return rc;
}

size_t termReadLink(const char* path, char* buffer, size_t bufsize)
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD result = 0;
    char* file = NULL;
    if (!path || !buffer || bufsize == 0)
    {
        errno = EINVAL;
        return -1;
    }
    hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        result = GetFinalPathNameByHandleA(hFile, buffer, (DWORD)bufsize, FILE_NAME_NORMALIZED);
        if (result == 0 || result > bufsize)
        {
            CloseHandle(hFile);
            fprintf(stderr, "Failed to get the full path name: %d\n", GetLastError());
            return -1;
        }
        CloseHandle(hFile);
    }
    return result;
}

int addCommandHistory(struct command_history** cmd_history, char* command, size_t length)
{
    struct command_history* ptr = NULL;
    if (!cmd_history || !command || length <= 0)
    {
        errno = EINVAL;
        return -1;
    }
    uint32_t hash = calculateHash(command, length) ;
    ptr = *cmd_history;
    if (ptr)
    {
        while (ptr->next)
        {
            if (ptr->checksum == hash)
            {
                return 0;
            }
            ptr = ptr->next;
        }
        ptr->next = (struct command_history*)malloc(sizeof(struct command_history));
        if (!ptr->next)
        {
            errno = ENOMEM;
            return -1;
        }
        memset(ptr->next, 0, sizeof(struct command_history));
        ptr->next->prev = ptr;
        ptr = ptr->next;
        
    }
    else
    {
        ptr = (struct command_history*)malloc(sizeof(struct command_history));
        if (!ptr)
        {
            errno = ENOMEM;
            return -1;
        }
        memset(ptr, 0, sizeof(struct command_history));
    }
    ptr->command = (char*)malloc(length + 1);
    if (!ptr->command)
    {
        errno = ENOMEM;
        return -1;
    }
    memcpy(ptr->command, command, length);
    ptr->command[length] = '\0';
    ptr->length = length;
    ptr->checksum = hash;
    ptr->lastUsed = time(NULL);
    command_history_count++;
    if (command_history_count > 10)
    {
        deleteOldestHistory(cmd_history);
    }
    if (*cmd_history == NULL)
    {
        *cmd_history = ptr;
    }
    return 0;
}

int deleteOldestHistory(struct command_history** cmd_history)
{
    struct command_history* ptr = NULL;
    struct command_history* oldestHistory = NULL;
    time_t oldest = time(NULL);
    if (cmd_history && *cmd_history)
    {
        ptr = *cmd_history;
        while (ptr)
        {
            if (ptr->lastUsed < oldest)
            {
                oldestHistory = ptr;
                oldest = ptr->lastUsed;
            }
            ptr = ptr->next;
        }
        if (oldestHistory)
        {
            ptr = oldestHistory;
            ptr->prev->next = ptr->next;
            ptr->next->prev = ptr->prev;
            if (ptr->command)
            {
                free(ptr->command);
                ptr->command = NULL;
            }
            free(ptr);
        }
        return 0;
    }
    errno = EINVAL;
    return -1;
}

uint32_t calculateHash(char * command, size_t length)
{
    uint64_t value = 0;
    uint8_t index = 0;
    char* ptr = NULL;
    if (command != NULL)
    {
        ptr = command;
        while (*ptr != '\0')
        {
            index++;
            value += index * (*ptr);
            ptr++;
        }
        return value & 0xFFFF;
    }
    return 0;
}

void cleanHistory(struct command_history** cmd_history)
{
    struct command_history* ptr = NULL;
    struct command_history* save = NULL;
    if (cmd_history && *cmd_history)
    {
        ptr = *cmd_history;
        while (ptr)
        {
            save = ptr->next;
            if (ptr->command)
            {
                free(ptr->command);
                ptr->command = NULL;
            }
            free(ptr);
            ptr = save;
        }
    }
}